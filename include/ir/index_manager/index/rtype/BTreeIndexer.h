#ifndef IZENELIB_IR_BTREEINDEXER_H_
#define IZENELIB_IR_BTREEINDEXER_H_

#include <util/BoostVariantUtil.h>
#include <am/luxio/BTree.h>
#include <am/tc/BTree.h>
#include <am/leveldb/Table.h>
#include <am/range/AmIterator.h>
#include <ir/index_manager/index/rtype/InMemoryBTreeCache.h>
#include <ir/index_manager/index/rtype/TermEnum.h>
#include <ir/index_manager/index/rtype/Compare.h>
#include <ir/index_manager/index/IndexerDocument.h>
#include <ir/index_manager/utility/Bitset.h>
#include <ir/index_manager/store/Directory.h>
#include <util/ClockTimer.h>
#include <glog/logging.h>

#include <boost/variant.hpp>
#include <boost/serialization/variant.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/type_traits.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/not.hpp>
#include <boost/bind.hpp>
#if BOOST_VERSION >= 105300
#include <boost/atomic.hpp>
#include <3rdparty/folly/RWSpinLock.h>
#endif
#include <util/ReadFavorLock.h>
#include "dynamic_bitset2.hpp"
#include <functional>

#include <algorithm>
#include <string>
#include <map>

//#define CACHE_DEBUG
//#define BT_DEBUG
//#define BT_INFO
//#define DOCS_INFO
//#define CACHE_TIME_INFO

NS_IZENELIB_IR_BEGIN

namespace indexmanager
{
/**
 * BTreeIndexer
 * @brief BTreeIndexer is used for BTree search and range search.
 */

template <class KeyType>
class BTreeIndexer
{
    typedef BTreeIndexer<KeyType> ThisType;
public:
    typedef std::vector<docid_t> DocListType;
    typedef boost::variant<DocListType, Bitset> ValueType;
    typedef boost::mutex WriteOnlyMutex;
    typedef izenelib::util::ReadFavorLock<500> MutexType;
    typedef izenelib::am::leveldb::Table<KeyType, ValueType> DbType;
    typedef InMemoryBTreeCache<KeyType, docid_t, MutexType> CacheType;
    typedef typename CacheType::ValueType CacheValueType;

    typedef std::map<KeyType, ValueType> PreLoadCacheType;

    typedef BTTermEnum<KeyType, CacheValueType, typename CacheType::AMType> MemEnumType;
    typedef AMTermEnum<DbType> AMEnumType;
    typedef PreLoadTermEnum<KeyType, ValueType, PreLoadCacheType> PreLoadTermEnumType;

    typedef TwoWayTermEnum<KeyType, CacheValueType, typename CacheType::AMType, DbType, ValueType> EnumType;
    typedef TwoWayPreLoadTermEnum<KeyType, CacheValueType, typename CacheType::AMType, PreLoadCacheType, ValueType> TwoWayPreLoadEnumType;

    typedef TermEnum<KeyType, ValueType> BaseEnumType;
    typedef boost::function<void (const CacheValueType&,const ValueType&, ValueType&) > EnumCombineFunc;
    typedef boost::dynamic_bitset2<uint32_t> DynBitsetType;
    typedef izenelib::am::AMIterator<DbType> iterator;
    static const size_t MAX_VALUE_LEN = 1024*1024;

    BTreeIndexer(const std::string& path, const std::string& property_name,
        bool pre_load = false, std::size_t cacheSize = 2000000)//an experienced value
        : path_(path), property_name_(property_name), mutex_(), cache_(mutex_), count_has_modify_(false)
    {
        cache_.set_max_capacity(cacheSize);
        func_ = &combineValue_;
        pre_load_ = pre_load;
    }

    ~BTreeIndexer()
    {
    }

    bool open()
    {
        if(!db_.open(path_))
            return false;
        if (pre_load_)
        {
            std::cerr << "begin preload btree filter for property: " << property_name_ << std::endl;

            pre_loaded_data_.clear();
            std::auto_ptr<BaseEnumType> term_enum(getAMEnum_());
            std::pair<KeyType, ValueType> kvp;
            while (term_enum->next(kvp))
            {
                pre_loaded_data_[kvp.first] = kvp.second;
            }
            std::cerr << "total preloaded : " << pre_loaded_data_.size();
        }
        return true;
    }

    void close()
    {
        db_.close();
    }

    iterator begin()
    {
        return iterator(db_);
    }

    iterator end()
    {
        return iterator();
    }

    static size_t  getValueNum(const ValueType& val)
    {
        if (val.which() == 0)
            return boost::get<DocListType>(val).size();
        else
            return boost::get<Bitset>(val).count();
    }

    static bool  isEmpltyValue(const ValueType& val)
    {
        if (val.which() == 0)
            return boost::get<DocListType>(val).empty();
        else
            return !boost::get<Bitset>(val).any();
    }

    static docid_t getDocId(const ValueType& val, size_t index)
    {
        if (val.which() == 0)
        {
            // value is common docid vector.
            return boost::get<DocListType>(val)[index];
        }
        else
        {
            // value is bitvector
            const Bitset& tmp = boost::get<Bitset>(val);
            size_t num = 0;
            for(std::size_t i = 0; i < tmp.size(); i++)
            {
                if (tmp.test(i)) ++num;
                if (num == index + 1)
                    return i;
            }
            return 0;
        }
    }

    void add(const KeyType& key, docid_t docid)
    {
    	{
            boost::unique_lock<WriteOnlyMutex> lock(mutex2_);
            cache_.add(key, docid);
    	}
        checkCache_();
        count_has_modify_ = true;
    }

    void remove(const KeyType& key, docid_t docid)
    {
        {
            boost::unique_lock<WriteOnlyMutex> lock(mutex2_);
            cache_.remove(key, docid);
    	}
        checkCache_();
        count_has_modify_ = true;
    }

    std::size_t count()
    {
        boost::upgrade_lock<MutexType> lock(mutex_);
        if (count_has_modify_ || !count_in_cache_)
        {
            boost::upgrade_to_unique_lock<MutexType> unique_lock(lock);
            if (count_has_modify_ || !count_in_cache_)
            {
                std::size_t db_count = getCount_();
                count_in_cache_ = db_count;
                count_has_modify_ = false;
//              LOG(INFO) << property_name_ << " count NOT in cache : " << db_count << std::endl;
            }
        }
        std::size_t count = count_in_cache_.get();
//      LOG(INFO) << property_name_ << " count in cache : " << count << std::endl;
        return count;

    }

    bool seek(const KeyType& key)
    {
        boost::shared_lock<MutexType> lock(mutex_);
        Bitset docs;
        getValue_(key, docs);
        return docs.any();
    }

    void getNoneEmptyList(const KeyType& key, Bitset& docs)
    {
        boost::shared_lock<MutexType> lock(mutex_);
        getValue_(key, docs);
    }

    void getValue(const KeyType& key, Bitset& docs)
    {
        boost::shared_lock<MutexType> lock(mutex_);
        getValue_(key, docs);
    }

    bool getValue(const KeyType& key, ValueType& docs)
    {
        boost::shared_lock<MutexType> lock(mutex_);
        return getValue_(key, docs);
    }

    std::size_t convertAllValue(std::size_t maxDoc, uint32_t* & data);

    //std::size_t getValueBetween(const KeyType& lowKey, const KeyType& highKey, std::size_t maxDoc, KeyType* & data)
    //{
    //    if (compare_(lowKey, highKey) > 0) return 0;
    //    boost::shared_lock<MutexType> lock(mutex_);
    //    std::size_t result = 0;

    //    std::auto_ptr<BaseEnumType> term_enum(getEnum_(lowKey));
    //    std::pair<KeyType, ValueType> kvp;
    //    docid_t docid = 0;
    //    while (term_enum->next(kvp))
    //    {
    //        if (compare_(kvp.first, highKey) > 0) break;
    //        for (uint32_t i = 0; i < getValueNum(kvp.second); i++)
    //        {
    //            docid = getDocId(kvp.second, i);
    //            if (docid >= maxDoc) break;
    //            data[docid] = kvp.first;
    //            ++result;
    //        }
    //    }

    //    return result;
    //}

    void getValueBetween(const KeyType& key1, const KeyType& key2, Bitset& docs)
    {
        if (compare_(key1, key2) > 0) return;
        boost::shared_lock<MutexType> lock(mutex_);

        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key1));
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (compare_(kvp.first, key2) > 0) break;
            decompress_(kvp.second, docs);
        }
    }

    void getValueLess(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] "<< docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (compare_(kvp.first, key) >= 0) break;
            decompress_(kvp.second, docs);
        }
    }

    void getValueLessEqual(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (compare_(kvp.first, key) > 0) break;
            decompress_(kvp.second, docs);
        }
    }

    void getValueGreat(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (compare_(kvp.first, key) == 0) continue;
            decompress_(kvp.second, docs);
        }
    }

    void getValueGreatEqual(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            decompress_(kvp.second, docs);
        }
    }


    void getValueStart(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_(key));
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (!Compare<KeyType>::start_with(kvp.first, key)) break;
            decompress_(kvp.second, docs);
        }
    }

    void getValueEnd(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (!Compare<KeyType>::end_with(kvp.first, key)) continue;
            decompress_(kvp.second, docs);
        }
    }

    void getValueSubString(const KeyType& key, Bitset& docs)
    {
#ifdef DOCS_INFO
        std::cout << "[start] " << docs << std::endl;
#endif
        boost::shared_lock<MutexType> lock(mutex_);
        std::auto_ptr<BaseEnumType> term_enum(getEnum_());
        std::pair<KeyType, ValueType> kvp;
        while (term_enum->next(kvp))
        {
            if (!Compare<KeyType>::contains(kvp.first, key)) continue;
            decompress_(kvp.second, docs);
        }
    }

    void flush()
    {
        cacheClear_();
    }


private:

    template<typename T>
    static int compare_(const T& t1, const T& t2)
    {
        return Compare<T>::compare(t1, t2);
    }

    bool cacheEmpty_()
    {
        return cache_.empty();
    }

    BaseEnumType* getEnum_(const KeyType& lowKey)
    {
        BaseEnumType* term_enum = NULL;
        if (cacheEmpty_())
        {
            if (pre_load_)
            {
                term_enum = getPreLoadEnum_(lowKey);
            }
            else
            {
                term_enum = getAMEnum_(lowKey);
            }
        }
        else
        {
            if (pre_load_)
            {
                term_enum = new TwoWayPreLoadEnumType(cache_.getAM(), pre_loaded_data_, lowKey, func_);
            }
            else
            {
                term_enum = new EnumType(cache_.getAM(), db_, lowKey, func_);
            }
        }
        return term_enum;
    }

    BaseEnumType* getEnum_()
    {
        BaseEnumType* term_enum = NULL;
        if (cacheEmpty_())
        {
            if (pre_load_)
            {
                term_enum = getPreLoadEnum_();
            }
            else
            {
                term_enum = getAMEnum_();
            }
        }
        else
        {
            if (pre_load_)
            {
                term_enum = new TwoWayPreLoadEnumType(cache_.getAM(), pre_loaded_data_, func_);
            }
            else
            {
                term_enum = new EnumType(cache_.getAM(), db_, func_);
            }
        }
        return term_enum;
    }

    MemEnumType* getMemEnum_(const KeyType& lowKey)
    {
        MemEnumType* term_enum = new MemEnumType(cache_.getAM(), lowKey);
        return term_enum;
    }

    MemEnumType* getMemEnum_()
    {
        MemEnumType* term_enum = new MemEnumType(cache_.getAM());
        return term_enum;
    }

    PreLoadTermEnumType* getPreLoadEnum_(const KeyType& lowKey)
    {
        PreLoadTermEnumType* term_enum = new PreLoadTermEnumType(pre_loaded_data_, lowKey);
        return term_enum;
    }

    PreLoadTermEnumType* getPreLoadEnum_()
    {
        PreLoadTermEnumType* term_enum = new PreLoadTermEnumType(pre_loaded_data_);
        return term_enum;
    }

    AMEnumType* getAMEnum_(const KeyType& lowKey)
    {
        AMEnumType* term_enum = new AMEnumType(db_, lowKey);
        return term_enum;
    }

    AMEnumType* getAMEnum_()
    {
        AMEnumType* term_enum = new AMEnumType(db_);
        return term_enum;
    }

    void checkCache_()
    {
#ifdef BT_INFO
        if (cache_.capacity() % 500000 == 0)
        {
            std::cout << "!!!cache status : " << property_name_ << ", size: " << cache_.capacity() << std::endl;
        }
#endif
        if (cache_.is_full())
        {
            cacheClear_();
        }
    }

    void cacheClear_()
    {
#ifdef BT_INFO
        std::cout << "!!!cacheClear_ " << property_name_ << ", key size: " << cache_.key_size() << ", size: " << cache_.capacity() << std::endl;
        cache_num_ = 0;
#endif
        boost::unique_lock<WriteOnlyMutex> lock2(mutex2_);
        cache_.iterate(boost::bind(&ThisType::cacheIterator_, this, _1));
        boost::unique_lock<MutexType> lock(mutex_);
        cache_.clear();
        db_.flush();
        count_has_modify_ = true;//force use db_.size() in count as cache will be empty
    }

    void cacheIterator_(const std::pair<KeyType, CacheValueType>& kvp)
    {
#ifdef CACHE_TIME_INFO
        static double t1 = 0.0;
        static double t2 = 0.0;
        static double t3 = 0.0;
//      std::cout << "start buffer size : " << common_compressed_.bufferCapacity() << std::endl;
        izenelib::util::ClockTimer timer;
#endif


        ValueType common_value;
#ifdef CACHE_DEBUG
        std::cout << "cacheIterator : " << kvp.first << "," << kvp.second << std::endl;
#endif

#ifdef CACHE_TIME_INFO
//      std::cout << "initialzed buffer size : " << common_compressed_.bufferCapacity() << std::endl;
        timer.restart();
#endif
        getDbValue_(kvp.first, common_value);
#ifdef CACHE_TIME_INFO
        t1 += timer.elapsed();
//      std::cout << "deserilized buffer size : " << common_compressed_.bufferCapacity() << std::endl;
#endif

#ifdef CACHE_DEBUG
        std::cout << "cacheIterator before update : " << kvp.first << "," << value << std::endl;
#endif

#ifdef CACHE_TIME_INFO
        timer.restart();
#endif
        applyCacheValue_(common_value, kvp.second);
#ifdef CACHE_TIME_INFO
        t2 += timer.elapsed();
#endif

#ifdef CACHE_TIME_INFO
        timer.restart();
#endif




#ifdef CACHE_DEBUG
        std::cout << "cacheIterator after update : " << kvp.first << "," << value << std::endl;
#endif
#ifdef CACHE_TIME_INFO
        timer.restart();
#endif
//      db_.update(kvp.first, common_compressed_);
//#ifdef CACHE_TIME_INFO
//      t3 += timer.elapsed();
//#endif

        size_t value_size = 0;
        if (common_value.which() == 0)
        {
            DocListType& tmp = boost::get<DocListType>(common_value);
            value_size = tmp.size();
            if (value_size > MAX_VALUE_LEN)
            {
                // too much value, convert it to bitvector to save space.
                Bitset newvalue;
                for(size_t i = 0; i < tmp.size(); ++i)
                {
                    newvalue.set(tmp[i]);
                }
                std::cerr << "btree index value converted to Bitset since the list is too large."
                    << ", key: " << kvp.first << ", value num: " << tmp.size() << std::endl;
                common_value = newvalue;
            }
        }

        boost::unique_lock<MutexType> lock(mutex_);
        if ((common_value.which() == 1) || value_size > 0)
        {
            db_.update(kvp.first, common_value);
            if (pre_load_)
                pre_loaded_data_[kvp.first] = common_value;
        }
        else
        {
            db_.del(kvp.first);
            if (pre_load_)
                pre_loaded_data_.erase(kvp.first);
        }
        cache_.clear_key(kvp.first);
#ifdef CACHE_TIME_INFO
        t3 += timer.elapsed();
#endif


#ifdef CACHE_DEBUG
        std::cout << "cacheIterator update finish" << std::endl;
#endif
#ifdef BT_INFO
        ++cache_num_;
        if (cache_num_ % 10000 == 0)
        {
            std::cout << "cacheIterator number : " << cache_num_ << std::endl;
        }
#endif
#ifdef CACHE_TIME_INFO
        if (cache_num_ % 10000 == 0)
        {
            LOG(INFO)<<"cacheIterator "<<cache_num_<<" time cost : "<<t1<<","<<t2<<","<<t3<<std::endl;
        }
#endif
    }

    bool getDbValue_(const KeyType& key, ValueType& value)
    {
        if (pre_load_)
        {
            typename PreLoadCacheType::const_iterator it = pre_loaded_data_.find(key);
            if (it == pre_loaded_data_.end())
                return false;
            value = it->second;
            return true;
        }
        return db_.get(key, value);
    }

    bool getPreLoadDbValue_(const KeyType& key, const ValueType*& value)
    {
        if (pre_load_)
        {
            typename PreLoadCacheType::const_iterator it = pre_loaded_data_.find(key);
            if (it == pre_loaded_data_.end())
                return false;
            value = &(it->second);
            return true;
        }
        return false;
    }

    bool getCacheValue_(const KeyType& key, CacheValueType& value)
    {
        bool b = cache_.get(key, value);
        if(value.empty()) b = false;
        return b;
    }

    // lock should be outside !
    std::size_t getCount_()
    {
        if (cache_.empty())
        {
            if (pre_load_)
                return pre_loaded_data_.size();
            return db_.size();
        }
        else {
            std::size_t count = 0;
            std::auto_ptr<BaseEnumType> term_enum(getEnum_());
            std::pair<KeyType, ValueType> kvp;
            while (term_enum->next(kvp))
            {
                if (!isEmpltyValue(kvp.second))
                {
                    ++count;
                }
            }
            return count;
        }
    }

    bool getValue_(const KeyType& key, Bitset& value)
    {
        const ValueType* compressed = NULL;
        ValueType tmp;
        bool b_db = false;
        if (pre_load_)
            b_db = getPreLoadDbValue_(key, compressed);
        else
        {
            b_db = getDbValue_(key, tmp);
            compressed = &tmp;
        }
        CacheValueType cache_value;
        bool b_cache = getCacheValue_(key, cache_value);
        if (!b_db && !b_cache) return false;
        decompress_(*compressed, value);
        if (b_cache)
        {
            applyCacheValue_(value, cache_value);
        }
        return true;
    }

    //bool getValue_(const KeyType& key, DocListType& value)
    //{
    //    ValueType dbvalue;
    //    bool b_db = getDbValue_(key, dbvalue);
    //    CacheValueType cache_value;
    //    bool b_cache = getCacheValue_(key, cache_value);
    //    if (!b_db && !b_cache) return false;
    //    if (dbvalue.which() == 0)
    //    {
    //        value = boost::get<DocListType>(dbvalue);
    //    }
    //    else
    //    {
    //        const Bitset& tmp = boost::get<Bitset>(dbvalue);
    //        value.reserve(MAX_VALUE_LEN);
    //        for(size_t i = 0; i < tmp.size(); ++i)
    //        {
    //            if (tmp.test(i))
    //                value.push_back(i);
    //        }
    //    }
    //    if (b_cache)
    //    {
    //        applyCacheValue_(value, cache_value);
    //    }
    //    if (value.size() > (size_t)MAX_VALUE_LEN)
    //    {
    //        std::cerr << "============= Waring: Btree value len is too large for vector!" <<
    //           " you should get this value using Bitset instead. ========= " << std::endl;
    //    }
    //    return true;
    //}

    bool getValue_(const KeyType& key, ValueType& value)
    {
        bool b_db = getDbValue_(key, value);
        CacheValueType cache_value;
        bool b_cache = getCacheValue_(key, cache_value);
        if (!b_db && !b_cache) return false;
        if (b_cache)
        {
            applyCacheValue_(value, cache_value);
        }
        return true;
    }

    static void decompress_(const ValueType& compressed, Bitset& value)
    {
        if (compressed.which() == 0)
        {
            const DocListType& tmp = boost::get<DocListType>(compressed);
            for (uint32_t i = 0; i < tmp.size(); i++)
            {
                value.set(tmp[i]);
            }
        }
        else
        {
            value |= boost::get<Bitset>(compressed);
        }
    }

    inline static void reset_common_bv_(DynBitsetType& value)
    {
        value.reset();
    }

    inline static void reset_common_bv_(Bitset& value)
    {
        value.reset();
    }


    /// Be called in range search function.
    static void combineValue_(const CacheValueType& cacheValue, const ValueType& value, ValueType& result)
    {
#ifdef BT_DEBUG
        std::cout << "before decompress" << std::endl;
        std::cout << result << std::endl;
        std::cout << cbitmap << std::endl;
#endif
        //decompress_(value, result);
        result = value;

#ifdef BT_DEBUG
        std::cout << "after decompress" << std::endl;
        std::cout << result << std::endl;
        std::cout << cacheValue << std::endl;
#endif
        applyCacheValue_(result, cacheValue);

#ifdef BT_DEBUG
        std::cout << "after combine" << std::endl;
        std::cout << result << std::endl;
#endif
    }

    /// called only in cacheIterator_, one thread, common_bv_ is safe.
    /// value was already sorted, also cacheValue was sorted
    static void applyCacheValue_(DocListType& value, const CacheValueType& cacheValue)
    {
        DocListType new_value;
        new_value.reserve(value.size()+cacheValue.size()/2);
        DocListType::const_iterator it1 = value.begin();
        typename CacheValueType::const_iterator it2 = cacheValue.begin();
        typename CacheValueType::const_iterator it2_end = cacheValue.end();
        while(it1!=value.end()&&it2!=it2_end)
        {
            if(*it1<*it2)
            {
                new_value.push_back(*it1);
                ++it1;
            }
            else if(*it1>*it2)
            {
                if(it2.test())//is insert, not delete
                {
                    new_value.push_back(*it2);
                }
                ++it2;
            }
            else
            {
                if(it2.test())
                {
                    new_value.push_back(*it1);
                }
                ++it1;
                ++it2;
            }
        }
        for(;it1!=value.end(); ++it1)
        {
            new_value.push_back(*it1);
        }
        for(;it2!=it2_end;++it2)
        {
            if(it2.test())
            {
                new_value.push_back(*it2);
            }
        }
        value.swap(new_value);
    }

    static void applyCacheValue_(ValueType& value, const CacheValueType& cacheValue)
    {
        if (value.which() == 0)
        {
            applyCacheValue_(boost::get<DocListType>(value), cacheValue);
        }
        else
        {
            applyCacheValue_(boost::get<Bitset>(value), cacheValue);
        }
    }

    static void applyCacheValue_(Bitset& value, const CacheValueType& cacheValue)
    {
        std::size_t count = cacheValue.count;
        for (std::size_t i = 0; i < count; i++)
        {
            if (cacheValue.item[i].second)
            {
                value.set(cacheValue.item[i].first);
            }
            else
            {
                value.reset(cacheValue.item[i].first);
            }
        }
    }

private:
    std::string path_;
    std::string property_name_;
    DbType db_;
    MutexType mutex_;
    WriteOnlyMutex mutex2_;
    CacheType cache_;
    EnumCombineFunc func_;
    /// used only in cacheIterator_, one thread,  safe.
    DynBitsetType common_bv_;
    ///
    boost::optional<std::size_t> count_in_cache_;
#if BOOST_VERSION >= 105300
    boost::atomic_bool count_has_modify_;
#else
    bool count_has_modify_;
#endif
    std::size_t cache_num_;
    bool pre_load_;
    PreLoadCacheType pre_loaded_data_;
};

}

NS_IZENELIB_IR_END

#endif /*BTreeIndex_H*/
