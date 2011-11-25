#ifndef IZENE_NET_DISTRIBUTE_MSG_H__
#define IZENE_NET_DISTRIBUTE_MSG_H__

#include <util/kv2string.h>
#include <string.h>

using namespace izenelib::util;

namespace net{
namespace distribute{

/**
 * Message struct for transferring data
 * | header |  data  |
 *
 */
struct MsgHead
{
    static const char* MSG_KEY_FILETYPE;
    static const char* MSG_KEY_FILENAME;
    static const char* MSG_KEY_DATALENGTH;
    static const char* MSG_KEY_COLLECTION;

    static const char* MSG_KEY_STATUS;
    static const char* MSG_KEY_ERRORINFO;
    static const char* MSG_KEY_RECV_DATALENGTH;

    static const char* MSG_ERROR_OK;
    static const char* MSG_ERROR_FAILEDTOCREATE;

    /**
     * msgRef().setValue(k, v)
     * @return ref to msg
     */
    kv2string& msgRef()
    {
        return msgpack_;
    }

    std::string toString()
    {
        return msgpack_.serialize();
    }

    std::string toStringWithEnd()
    {
        char* eoh = NULL;
        size_t len;
        getEndOfHeader(eoh, len);

        return msgpack_.serialize()+eoh;
    }

    void loadMsg(std::string msg)
    {
        msgpack_.loadKvString(msg);
    }

    static void getEndOfHeader(char*& eoh, size_t& len)
    {
        static char s_eoh[] = "\r\n\r\n";
        eoh = s_eoh;
        len = strlen(s_eoh);
    }

    kv2string msgpack_;
};

struct SendFileReqMsg : public MsgHead
{
    enum eFileType
    {
        FTYPE_SCD,
        FTYPE_BIN
    };

    void setFileType(eFileType etype)
    {
        std::string stype;
        switch (etype)
        {
            case FTYPE_SCD:
                stype = "scd";
                break;
            case FTYPE_BIN:
                stype = "bin";
                break;
            default:
                stype = "undefined";
                break;
        }

        msgpack_.setValue(MSG_KEY_FILETYPE, stype);
    }

    bool isSCD()
    {
        if ( msgpack_.hasKey(MSG_KEY_FILETYPE)
             && msgpack_.getStrValue(MSG_KEY_FILETYPE) == "scd")
        {
            return true;
        }
        return false;
    }

    void setCollection(const std::string& collection)
    {
        msgpack_.setValue(MSG_KEY_COLLECTION, collection);
    }

    std::string getCollection()
    {
        return msgpack_.getStrValue(MSG_KEY_COLLECTION);
    }

    void setFileName(const std::string& fileName)
    {
        msgpack_.setValue(MSG_KEY_FILENAME, fileName);
    }

    std::string getFileName()
    {
        return msgpack_.getStrValue(MSG_KEY_FILENAME);
    }

    void setFileSize(unsigned int fileSize)
    {
        msgpack_.setValue(MSG_KEY_DATALENGTH, fileSize);
    }

    unsigned int getFileSize()
    {
        return msgpack_.getUIntValue(MSG_KEY_DATALENGTH);
    }
};



struct ResponseMsg : public MsgHead
{
    void setStatus(const std::string& stat)
    {
        msgpack_.setValue(MSG_KEY_STATUS, stat);
    }

    std::string getStatus()
    {
        return msgpack_.getStrValue(MSG_KEY_STATUS);
    }

    void setReceivedSize(unsigned int fileSize)
    {
        msgpack_.setValue(MSG_KEY_RECV_DATALENGTH, fileSize);
    }

    unsigned int getReceivedSize()
    {
        return msgpack_.getUIntValue(MSG_KEY_RECV_DATALENGTH);
    }
};

}
}

#endif /* IZENE_NET_DISTRIBUTE_MSG_H__ */