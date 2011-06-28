#include <net/aggregator/JobAggregator.h>
#include <net/aggregator/AggregatorConfig.h>
#include "data_type.h"

using namespace net::aggregator;


class SearchAggregator : public JobAggregator<SearchAggregator>
{
public:
    void join_impl(Data& res, const Data& workerRes)
    {
        cout << "join_impl" <<endl;
        res.s += workerRes.s;
    }

    void join_impl(DataResult& res, const std::vector<DataResult>& resList)
    {
        cout << "join_impl 2" <<endl;

        for (size_t i = 0; i < resList.size(); i++)
        {
            res.s += resList[i].s;
        }
        //sleep(6);
    }
};

int main( int argc, char * argv[])
{
    AggregatorConfig config;
    config.addWorker("0.0.0.0", 18111);
    config.addWorker("0.0.0.0", 18112);
    config.addWorker("0.0.0.0", 18113);

    SearchAggregator ag;
    ag.setWorkerListConfig(config);

    Data req;
    //Data ret;
    DataResult ret;
    ag.processRequest<Data, DataResult>("getKeywordSearchResult", req, ret);

    std::cout << "join: "<<ret.i << "/" <<ret.s<< std::endl;
}

