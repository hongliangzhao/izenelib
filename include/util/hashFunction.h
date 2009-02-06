//-*-C++-*-
/**
 * @file  HashFunction.h
 * @brief The definitions of hashing functions used in
 *        hashing methods.
 */
#ifndef HashFunction_h
#define HashFunction_h 1
#include <stdlib.h>


NS_IZENELIB_UTIL_BEGIN

typedef  uint64_t  ub8;         /** unsigned 8-byte quantities */
typedef  uint32_t  ub4;         /** unsigned 4-byte quantities */
typedef  uint16_t  ub2;         /** unsigned 2-byte quantities */
typedef  unsigned  char ub1;    /** unsigned 1-byte quantities */
#define hashsize(n) ((ub8)1<<(n))
#define hashmask(n) (hashsize(n)-1)
#define mix(a,b,c) \
{ \
  a -= b; a -= c; a ^= (c>>13); \
  b -= c; b -= a; b ^= (a<<8); \
  c -= a; c -= b; c ^= (b>>13); \
  a -= b; a -= c; a ^= (c>>12);  \
  b -= c; b -= a; b ^= (a<<16); \
  c -= a; c -= b; c ^= (b>>5); \
  a -= b; a -= c; a ^= (c>>3);  \
  b -= c; b -= a; b ^= (a<<10); \
  c -= a; c -= b; c ^= (b>>15); \
}
#define mix64(a,b,c) \
{ \
  a=a-b;  a=a-c;  a=a^(c>>43); \
  b=b-c;  b=b-a;  b=b^(a<<9); \
  c=c-a;  c=c-b;  c=c^(b>>8); \
  a=a-b;  a=a-c;  a=a^(c>>38); \
  b=b-c;  b=b-a;  b=b^(a<<23); \
  c=c-a;  c=c-b;  c=c^(b>>5); \
  a=a-b;  a=a-c;  a=a^(c>>35); \
  b=b-c;  b=b-a;  b=b^(a<<49); \
  c=c-a;  c=c-b;  c=c^(b>>11); \
  a=a-b;  a=a-c;  a=a^(c>>12); \
  b=b-c;  b=b-a;  b=b^(a<<18); \
  c=c-a;  c=c-b;  c=c^(b>>22); \
}


template <class KeyType>
/**
 * @brief The wrapper for all kinds of hashing functions.
 */
class HashFunction {
private:
  static const ub4 init_pattern_1 = 0x00000000;
  static const ub4 init_pattern_2 = 0x13572468;

  static ub4 calcHash (register const char *k, register uint32_t length, register uint32_t initval) {
    register uint32_t a,b,c,len;
    /* Set up the internal state */

    len = length;
    a = b = 0x9e3779b9;  /* the golden ratio; an arbitrary value */
    c = initval;         /* the previous hash value */
    /*---------------------------------------- handle most of the key */
    while (len >= 12)
      {
	a += (k[0] +((uint32_t)k[1]<<8) +((uint32_t)k[2]<<16) +((uint32_t)k[3]<<24));
	b += (k[4] +((uint32_t)k[5]<<8) +((uint32_t)k[6]<<16) +((uint32_t)k[7]<<24));
	c += (k[8] +((uint32_t)k[9]<<8) +((uint32_t)k[10]<<16)+((uint32_t)k[11]<<24));
	mix(a,b,c);
	k += 12; len -= 12;
      }
    /*------------------------------------- handle the last 11 bytes */
    c += length;
    switch(len) /* all the case statements fall through */
      {
      case 11: c+=((uint32_t)k[10]<<24);
      case 10: c+=((uint32_t)k[9]<<16);
      case 9 : c+=((uint32_t)k[8]<<8);
	/* the first byte of c is reserved for the length */
      case 8 : b+=((uint32_t)k[7]<<24);
      case 7 : b+=((uint32_t)k[6]<<16);
      case 6 : b+=((uint32_t)k[5]<<8);
      case 5 : b+=k[4];
      case 4 : a+=((uint32_t)k[3]<<24);
      case 3 : a+=((uint32_t)k[2]<<16);
      case 2 : a+=((uint32_t)k[1]<<8);
      case 1 : a+=k[0];
	/* case 0: nothing left to add */
      }
    mix(a,b,c);

    /*-------------------------------------------- report the result */
    return c;
  }

public:
  static const int PRIME = 1048583; // often used in computing hash value. Big prime number.

  static const ub8 DEFAULT_SEED_VALUE = 0x9e3779b97f4a7c13LL;
  static ub8 hashFN64(const KeyType& key, ub8 seed = DEFAULT_SEED_VALUE) {
    register ub1 *k = (ub1 *)key.c_str();
    register ub4 length = key.size();
    register ub8 a,b,c;
    register ub4 len;

    /* Set up the internal state */
    len = length;
    a = b = seed ;  /* the golden ratio; an arbitrary value */
    c = 0x7902cac39f392d18LL;

    /*---------------------------------------- handle most of the key */
    while (len >= 24)
      {
	a += (k[0] +((ub8)k[1]<<8) +((ub8)k[2]<<16) +((ub8)k[3]<<24)
	      +((ub8)k[4]<<32)+((ub8)k[5]<<40)+((ub8)k[6]<<48)+((ub8)k[7]<<56));
	b += (k[8] +((ub8)k[9]<<8) +((ub8)k[10]<<16) +((ub8)k[11]<<24)
	      +((ub8)k[12]<<32)+((ub8)k[13]<<40)+((ub8)k[14]<<48)+((ub8)k[15]<<56));
	c += (k[16] +((ub8)k[17]<<8) +((ub8)k[18]<<16)+((ub8)k[19]<<24)
	      +((ub8)k[20]<<32)+((ub8)k[21]<<40)+((ub8)k[22]<<48)+((ub8)k[23]<<56));
	mix(a,b,c);
	k += 24; len -= 24;
      }

    /*------------------------------------- handle the last 11 bytes */
    c += length;
    switch(len)              /* all the case statements fall through */
      {
      case 23 : c+=((ub8)k[22]<<56);
      case 22 : c+=((ub8)k[21]<<48);
      case 21 : c+=((ub8)k[20]<<40);
      case 20 : c+=((ub8)k[19]<<32);
      case 19 : c+=((ub8)k[18]<<24);
      case 18 : c+=((ub8)k[17]<<16);
      case 17 : c+=((ub8)k[16]<<8);
	/* the first byte of c is reserved for the length */
      case 16 : b+=((ub8)k[15]<<56);
      case 15 : b+=((ub8)k[14]<<48);
      case 14 : b+=((ub8)k[13]<<40);
      case 13 : b+=((ub8)k[12]<<32);
      case 12 : b+=((ub8)k[11]<<24);
      case 11 : b+=((ub8)k[10]<<16);
      case 10 : b+=((ub8)k[9]<<8);
      case 9 : b+=k[8];
      case 8 : a+=((ub8)k[7]<<56);
      case 7 : a+=((ub8)k[6]<<48);
      case 6 : a+=((ub8)k[5]<<40);
      case 5 : a+=((ub8)k[4]<<32);
      case 4 : a+=((ub8)k[3]<<24);
      case 3 : a+=((ub8)k[2]<<16);
      case 2 : a+=((ub8)k[1]<<8);
      case 1 : a+=k[0];
	/* case 0: nothing left to add */
      }
    mix(a,b,c);
    /*-------------------------------------------- report the result */
    return c;
  }

  static ub8 generateHash64 (const KeyType& key) {
    const char *token = key.c_str();
    const unsigned int len = key.size();
    ub8 id = 0L;
    ub8 id1, id2;
    id1 = calcHash (token, len, init_pattern_1);
    id2 = calcHash (token, len, init_pattern_2);
    id = (id1 << 32) | id2;

    return id;
  }

  static ub4 generateHash32 (const KeyType& key) {
    const char *token = key.c_str();
    const unsigned int len = key.size();

    return calcHash (token, len, init_pattern_1);
  }

  static ub4 convert_key(const KeyType& key) {
    register ub4 convkey = 0;
    register const char* str = (const char*)key.c_str();
    for (register size_t i = 0; i < key.size(); i++)
      convkey = 37*convkey + *str++;

    return convkey;
  }

  static ub4 djb2(const KeyType& key) {
    register ub4 h = 5381;
    register const char* str = (const char*)key.c_str();
    for (register size_t i = 0; i < key.size(); i++, str++)
      h = ((h << 5) + h) + *str++;
    return h;
  }

  static ub4 sdbm(const KeyType& key) {
    register ub4 h = 0;
    register const char* str = (const char*)key.c_str();
    for (register size_t i = 0; i < key.size(); i++)
      h = *str++ + (h << 6) + (h << 16) - h;
    return h;
  }
};

NS_IZENELIB_UTIL_END
#endif
