/**
 * @file JobAggregator.h
 * @author Zhongxia Li
 * @date Jun 23, 2011
 * @brief JobAggregator distribute Job to multiple Workers and gather results.
 */
#ifndef JOB_AGGREGATOR_H_
#define JOB_AGGREGATOR_H_

#include <boost/shared_ptr.hpp>
#include <3rdparty/msgpack/rpc/client.h>

#include "AggregatorConfig.h"
#include "JobInfo.h"


namespace net{
namespace aggregator{

/**
 * Job Aggregator base class
 * @brief Implement a concrete aggregator using inheritance, the following function must be implemented:
 * join_impl(DataType& result, const std::vector<DataType>& resultList);
 * join_impl() used to merge result, it's not virtual, because base class won't known the DataType
 * of a concrete aggregator.
 * @example

class SearchAggregator : public JobAggregator<SearchAggregator>
{
public:
    void join_impl(ResultType& result, const ResultType& singleWorkerResult)
    {
        // merge singleWorkerResult to result
    }
};

 */
template <typename ConcreteAggregator>
class JobAggregator
{
public:
    /// todo, remove
    JobAggregator()
    {
    }

    JobAggregator(const std::string& host, uint16_t port)
    : srvInfo_(host, port)
    {
    }

public:
    void setWorkerListConfig(AggregatorConfig& aggregatorConfig)
    {
        const std::vector<ServerInfo>& workerInfoList = aggregatorConfig.getWorkerList();

        for (size_t i = 0; i < workerInfoList.size(); i ++)
        {
            registerWorker(workerInfoList[i]);
        }
    }

    template <typename RequestParamType, typename ResultParamType>
    void sendRequest(
            const std::string& func,
            const RequestParamType& request,
            ResultParamType& result,
            unsigned int timeout = 2) // xxx
    {
        worker_iterator_t worker = workerSessionPool_.begin();
        for ( ; worker != workerSessionPool_.end(); worker++ )
        {
            (*worker)->sendRequest(func, request);
        }

        join(result);
    }

protected:
    void init()
    {

    }

    void registerWorker(const ServerInfo& srvInfo)
    {
        if (isLocalWorker(srvInfo)) {
            // xxx
        }
        else {
            createWorkerClient(srvInfo);
        }
    }

    bool isLocalWorker(const ServerInfo& srvInfo)
    {
        if (srvInfo.host_ == srvInfo_.host_
                && srvInfo.port_ == srvInfo.port_)
        {
            return true;
        }
        return false;
    }

    void createWorkerClient(const ServerInfo& srvInfo)
    {
        try
        {
            WorkerSessionPtr workerSession(new WorkerSession(srvInfo.host_, srvInfo.port_));
            workerSessionPool_.push_back(workerSession);
        }
        catch(std::exception& e)
        {
            std::cerr<<"Failed connecting to: "<<srvInfo.host_<<":"<<srvInfo.port_<<std::endl
                     <<e.what()<<std::endl;
        }
    }

protected:
    template <typename ResultParamType>
    void join(ResultParamType& result)
    {
        std::vector<ResultParamType> resultList;

        worker_iterator_t worker = workerSessionPool_.begin();
        for (; worker != workerSessionPool_.end(); worker++)
        {
            try
            {
                ResultParamType workerResult = (*worker)->getResult<ResultParamType>();
                resultList.push_back(workerResult);
                //static_cast<ConcreteAggregator*>(this)->join_impl(result, workerResult);
            }
            catch (msgpack::rpc::connect_error& e)
            {
                // xxx
                (*worker)->setError("connecting failed!");
            }
            catch (msgpack::rpc::timeout_error& e)
            {
                (*worker)->setError("request timed out!");
            }
            catch (msgpack::rpc::no_method_error& e)
            {
                (*worker)->setError("method not found!");
            }
            catch (msgpack::rpc::argument_error& e)
            {
                (*worker)->setError("argument mismatch!");
            }
            catch (msgpack::rpc::remote_error& e)
            {
                (*worker)->setError("remote_error");
            }
            catch (std::exception& e)
            {
                (*worker)->setError(e.what());
            }

            if ( !(*worker)->getState() )
            {
                cout <<"["<<(*worker)->getServerInfo().host_
                     <<":"<<(*worker)->getServerInfo().port_
                     <<"] "<< (*worker)->getError() << endl;
            }
        }

        if (resultList.size() > 0)
        {
            static_cast<ConcreteAggregator*>(this)->join_impl(result, resultList);
        }
        else
        {
            ;
        }
    }

private:
    ServerInfo srvInfo_;

    typedef std::vector<WorkerSessionPtr>::iterator worker_iterator_t;
    std::vector<WorkerSessionPtr> workerSessionPool_;
};

}} // end - namespace

#endif /* JOB_AGGREGATOR_H_ */