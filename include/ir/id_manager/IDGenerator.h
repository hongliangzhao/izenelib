/**-
 * @file	IDGenerator.h
 * @brief	Contain two types of IDGenerator, HashID and UniqueID.
 *          HashID use hash to genreate ID,
 *          UniqueID generate ID using a sequential number.
 * @author Wei Cao
 * @date 2009-08-07
 */

#ifndef _ID_GENERATOR_H_
#define _ID_GENERATOR_H_

#include <types.h>

#include <util/hashFunction.h>
#include <sdb/SequentialDB.h>

#include "IDFactoryException.h"
#include "IDFactoryErrorString.h"

#include "NameIDTraits.h"


NS_IZENELIB_IR_BEGIN

namespace idmanager {

template <typename  NameString,
          typename  NameID>
class EmptyIDGenerator
{
public:

	/**
	 * @brief Constructor.
	 */
	EmptyIDGenerator(const string&){}

	/**
	 * @brief Always return false, which means failure to generate ID
	 */
	inline bool conv(const NameString& nameString, NameID& nameID)
	{
        return false;
	}

	void display(){}

}; // end - template SequentialIDFactory



template <typename  NameString,
          typename  NameID,
          NameID    (*HashFunc)(const NameString&) = NameIDTraits<NameID>::hash>
class HashIDGenerator
{
public:

	/**
	 * @brief Constructor.
	 */
	HashIDGenerator(const string&){}

	/**
	 * @brief Convert String to ID, ID may be not unique
	 * @param nameString the name string
	 * @param nameID the NameID that may be not unique
	 * @return always false
	 */
	inline bool conv(const NameString& nameString, NameID& nameID)
	{
        nameID = HashFunc(nameString);
        return false;
	}

	void display(){}

}; // end - template SequentialIDFactory


template <
          typename  NameString,
          typename  NameID,
          NameID    MinIDValue  = NameIDTraits<NameID>::MinValue,
          NameID    MaxIDValue  = NameIDTraits<NameID>::MaxValue,
          typename  LockType    = izenelib::util::NullLock>
class UniqueIDGenerator
{
	typedef izenelib::sdb::ordered_sdb<NameString, NameID, LockType> IdFinder;

public:

	/**
	 * @brief Constructor.
	 *
	 * @param sdbName       name of sdb storage.
	 */
	UniqueIDGenerator(const string& sdbName);

	virtual ~UniqueIDGenerator();

	/**
	 * @brief This function returns a unique name id given a name string.
	 * @param nameString the name string
	 * @param nameID the unique NameID
	 * @return true if DocID already in dictionary
	 * @return false otherwise
	 */
	inline bool conv(const NameString& nameString, NameID& nameID);

	void display()
	{
		idFinder_.display();
	}

protected:

	NameID minID_; ///< An minimum ID.
	NameID maxID_; ///< An maximum ID.
	NameID newID_; ///< An ID for new name.
	string sdbName_;

	IdFinder idFinder_; ///< an indexer which gives ids according to the name.
}; // end - template SequentialIDFactory

template <typename NameString, typename NameID,
    NameID MinValueID, NameID MaxValueID, typename LockType>
UniqueIDGenerator<NameString, NameID,
    MinValueID, MaxValueID, LockType>::UniqueIDGenerator(
        const string& sdbName)
:
	minID_(MinValueID),
    maxID_(MaxValueID),
    newID_(MinValueID),
    sdbName_(sdbName),
    idFinder_(sdbName_ + "_name.sdb")
{
	idFinder_.open();

    // reset newID_
	if(idFinder_.numItems() > 0)
	{
	    NameID maxValue = MinValueID;
        NameString k; NameID v;
        typename IdFinder::SDBCursor locn = idFinder_.get_first_Locn();
        while (idFinder_.seq(locn, k, v) ) {
            if(maxValue < v)
                maxValue = v;
        }
        newID_ = maxValue + 1;
	}
} // end - SequentialIDFactory()

template <typename NameString, typename NameID,
    NameID MinValueID, NameID MaxValueID, typename LockType>
UniqueIDGenerator<NameString, NameID,
    MinValueID, MaxValueID, LockType>::~UniqueIDGenerator()
{
} // end - ~SequentialIDFactory()

template <typename NameString, typename NameID,
    NameID MinValueID, NameID MaxValueID, typename LockType>
inline bool UniqueIDGenerator<NameString, NameID,
    MinValueID, MaxValueID, LockType>::conv(
        const NameString& nameString,
        NameID& nameID)
{
	// If name string is found, return the id.
	if (idFinder_.getValue(nameString, nameID) ) {
		return true;
	} // end - if

	// Because there's no name string in idFinder, create new id according to the string.
	nameID = newID_;
	newID_++;

	// check correctness of input nameID
	if (newID_> maxID_)
		throw IDFactoryException(SF1_ID_FACTORY_OUT_OF_BOUND, __LINE__, __FILE__);

	idFinder_.insertValue(nameString, nameID);

	return false;
} // end - getNameIDByNameString()

}
// end - namespace idmanager

NS_IZENELIB_IR_END

#endif // #ifndef _HASH_ID_H_
