#ifndef IZENELIB_AM_H
#define IZENELIB_AM_H

#include <types.h>
#include <am/concept/DataType.h>
#include <util/ThreadModel.h>
#include <memory>

using namespace izenelib::util;
NS_IZENELIB_AM_BEGIN

template<typename KeyType, typename ValueType,
typename LockType=NullLock, typename Alloc=std::allocator<DataType<KeyType,ValueType> > >
class AccessMethod
{
public:
	virtual bool insert(const KeyType& key, const ValueType& value){
		DataType<KeyType,ValueType> data(key, value);
    	return insert(data);
	}

	virtual bool insert(const DataType<KeyType,ValueType>& data) = 0;

	virtual bool update(const KeyType& key, const ValueType& value){
		DataType<KeyType,ValueType> data(key, value);
    	return update(data);
	}

	virtual bool update(const DataType<KeyType,ValueType>& data) = 0;

	virtual ValueType* find(const KeyType& key) = 0;

	virtual bool del(const KeyType& key) = 0;

	virtual ~AccessMethod() {};
};

template<typename KeyType, typename LockType, typename Alloc >
class AccessMethod<KeyType, NullType, LockType, Alloc>
{
public:
	virtual bool insert(const KeyType& key){
		DataType<KeyType> data(key);
    	return insert(data);
	}

	virtual bool insert(const DataType<KeyType>& data) = 0;

	virtual bool update(const KeyType& key){
		DataType<KeyType> data(key);
    	return update(data);
	}

	virtual bool update(const DataType<KeyType>& data) = 0;


	virtual bool del(const KeyType& key) = 0;

	virtual ~AccessMethod() {};
};

NS_IZENELIB_AM_END

#endif //End of IZENELIB_AM_H