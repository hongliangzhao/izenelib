#ifndef _SDB_HASH2BTREE_H_
#define _SDB_HASH2BTREE_H_

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <ostream>

#include <am/external_sort/alpha_sort.hpp>

NS_IZENELIB_AM_BEGIN

template<typename UnorderedSdbType,
         typename OrderedSdbType>
class Conv_Sdb_Hash2Btree
{

typedef typename UnorderedSdbType::SDBKeyType KeyType;
typedef typename UnorderedSdbType::SDBValueType ValueType;

public:

Conv_Sdb_Hash2Btree(UnorderedSdbType& src, OrderedSdbType& dst, const std::string& name = "conv_sdb_hash2btree")
    :  fin_(name+".tmpi"), fout_(name+".tmpo"), src_sdb_(src), dst_sdb_(dst)
{
}

void startThread()
{
    std::remove(fin_.c_str());
    std::remove(fout_.c_str());
}

void mapStage()
{
    std::cout << "before map" << std::endl;
    FILE* f = fopen(fin_.c_str(), "w+");
    if(f==NULL)
        std::cerr << "bad file " << fin_ << std::endl;
        std::cout <<"0";
    size_t itemNum = src_sdb_.numItems();
    fwrite(&itemNum, sizeof(size_t), 1, f);
        std::cout <<"1";
    unsigned short itemSize = sizeof(KeyType) + sizeof(ValueType);
        std::cout <<"2";
    typename UnorderedSdbType::SDBCursor locn = src_sdb_.get_first_Locn();
        std::cout <<"3";
    KeyType k = KeyType();
    ValueType v = ValueType();
    while( src_sdb_.get(locn, k, v) ) {
        std::cout <<"4";
        fwrite(&itemSize, sizeof(unsigned short), 1, f);
        fwrite(&k, sizeof(KeyType), 1, f);
        fwrite(&v, sizeof(ValueType), 1, f);
        src_sdb_.seq(locn, k, v);
    }
//    while( src_sdb_.seq(locn, k, v) ) {
//        std::cout <<"4";
//        fwrite(&itemSize, sizeof(unsigned short), 1, f);
//        fwrite(&k, sizeof(KeyType), 1, f);
//        fwrite(&v, sizeof(ValueType), 1, f);
//    }

    fflush(f);
    fclose(f);
    std::cout << "after map" << std::endl;
    return;
}

void reduceStage1()
{
    AlphaSort<> alpha;

    alpha.addInputFile(fin_);

    clock_t start, finish;
    start = clock();
    alpha.sort(fout_);
    finish = clock();
#ifdef DEBUG
    std::cout << "external sort cost " <<
        (double)(finish - start) / CLOCKS_PER_SEC <<
        " seconds" << std::endl;
#endif
}

void reduceStage2()
{
    FILE* f = fopen(fout_.c_str(), "r");
    if(f==NULL)
        std::cerr << "bad file " << fout_ << std::endl;

    size_t itemNum;
    fread(&itemNum, sizeof(size_t), 1, f);
    if(itemNum != (size_t)src_sdb_.numItems())
        std::cerr<< "reduceStage2 itemNum should be " << src_sdb_.numItems()
            << " but is " << itemNum << std::endl;
#ifdef DEBUG
    unsigned short itemSize = sizeof(KeyType) + sizeof(ValueType);
#endif
    KeyType k = KeyType();
    ValueType v = ValueType();
    for( size_t i = 0; i < itemNum; i++ ) {
#ifdef DEBUG
        if(i%1000000 == 0)
            std::cout << "convert " << i << " elements" << std::endl;
#endif
        unsigned short tmp;
        fread(&tmp, sizeof(unsigned short), 1, f);
#ifdef DEBUG
        if(tmp != itemSize)
            std::cerr<< "reduceStage2 itemSize should be " << itemSize
                << " but is " << tmp << std::endl;
#endif
        fread(&k, sizeof(KeyType), 1, f);
        fread(&v, sizeof(ValueType), 1, f);
        dst_sdb_.insertValue(k, v);
    }
    fclose(f);
    return;
}

bool joinThread()
{
    mapStage();

    reduceStage1();

    reduceStage2();

    std::remove(fin_.c_str());
    std::remove(fout_.c_str());

    if(dst_sdb_.numItems() == src_sdb_.numItems())
        return true;
    return false;
}

protected:

std::string fin_;

std::string fout_;

UnorderedSdbType& src_sdb_;

OrderedSdbType& dst_sdb_;

};

NS_IZENELIB_AM_END

#endif