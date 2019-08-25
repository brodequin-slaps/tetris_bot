#pragma once

#include <functional>
#include <math.h>
#include <future>
#include <utility>
#include <math.h>
#include <thread>
#include <boost/fiber/all.hpp>
#include <future>

namespace LoadBalancer
{
    using namespace std;

    /*RunAsync takes a function that takes an int as parameter and returns the best result of the range of iteration based on the selector function
    compare must return -1 if A<B, 0 if A==B, 1 if A>B
    min and max args are the range of iteration*/
    template<typename T>
    T RunAsync(const std::function<T(const int&)>& func, const std::function<int(const T&, const T&)>& compare, const int& min, const int& max)
    {
        typedef boost::fibers::buffered_channel<T> channel_T;
        typedef boost::fibers::buffered_channel<int> channel_int;
        unsigned int range = max - min;
        //channel size must be a power of 2
        channel_T chan{1024};
        channel_int countChan{1024};

        int asyncCount;
        int numOSThreads = std::thread::hardware_concurrency();;
        if(range < numOSThreads)
        {
            asyncCount = range;
        }
        else
        {
            asyncCount = numOSThreads;
        }

        //create subranges of original range for the asyncs
        std::vector<std::pair<int, int>> subRanges;
        for(int i = 0; i < asyncCount - 1; i++)
        {
            subRanges.push_back(std::pair<int, int>(i*(floor(range/(double)asyncCount)), (i+1)*(floor(range/(double)asyncCount))));
        }
        subRanges.push_back(std::pair<int, int>((asyncCount - 1)*(floor(range/(double)asyncCount)), max));

        //launch all asyncs
        std::vector<std::future<void>> futures;
        for(int i = 0; i < asyncCount; i++)
        {
            futures.emplace_back(std::async(std::bind([func, compare, subRanges](const int& i, channel_T& chan, channel_int& countChan){
                countChan.push(1);
                T maxResult;
                bool initialized = false;
                for(int j = subRanges[i].first; j < subRanges[i].second; j++)
                {
                    if(!initialized)
                    {
                        initialized = true;
                        maxResult = func(j);
                    }
                    else
                    {
                        if(compare(func(j), maxResult) == 1)
                        {
                            maxResult = func(j);
                        }
                    }
                }
                chan.push(maxResult);
                countChan.push(-1);
            }, i, std::ref(chan), std::ref(countChan)), std::launch::async));
        }

        //launch the channel closing async
        auto closer = std::async(std::bind([=](channel_T& toClose, channel_int& countChan){
            int asyncsAlive = 0;
            int chanValuesCount = 0;
            for(auto& x : countChan)
            {
                chanValuesCount++;
                asyncsAlive += x;

                if(chanValuesCount >= 2*asyncCount && asyncsAlive <= 0)
                {
                    toClose.close();
                    countChan.close();
                    break;
                }
            }
        }, std::ref(chan), std::ref(countChan)), std::launch::async);

        //launch the merge async
        T ret;
        auto merger = std::async(std::bind([&](channel_T& chan){
            bool initialized = false;
            for(auto& x : chan)
            {
                if(!initialized)
                {
                    initialized = true;
                    ret = x;
                }
                else
                {
                    if(compare(x, ret) == 1)
                    {
                        ret = x;
                    }
                }
            }
        }, std::ref(chan)), std::launch::async);

        merger.wait();
        return ret;
    }


    template <typename T, typename InputIterator>
    void WorkloadGenerator(function<T(typename iterator_traits<InputIterator>::value_type)>& func, InputIterator first, InputIterator last, boost::fibers::buffered_channel<function<T()>>& chan_out, boost::fibers::buffered_channel<int>& closer)
    {
        for(auto it = first; it != last; ++it)
        {
            chan_out.push(function<T()>([&it, &func](){return func(*it);}));
        }
        closer.push(1);
    }

    template <typename T>
    void Worker(boost::fibers::buffered_channel<std::function<T()>>& chan_in, boost::fibers::buffered_channel<T>& chan_out, boost::fibers::buffered_channel<int>& closer)
    {
        for(auto& task : chan_in)
        {
            chan_out.push(task());
        }
        closer.push(1);
    }

    template <typename T>
    T Merger(const function<int(const T&, const T&)>& compare, boost::fibers::buffered_channel<T>& chan_in, boost::fibers::buffered_channel<T>& chan_out, boost::fibers::buffered_channel<int>& closer)
    {
        T best;
        bool initialized = false;
        for(auto val : chan_in)
        {
            if(!initialized)
            {
                initialized = true;
                best = val;
            }
            else
            {
                if(compare(val, best) == 1)
                {
                    best = val;
                }
            }
        }
        if(initialized)
            chan_out.push(best);
        closer.push(1);
    }

    template <typename T>
    void Closer(boost::fibers::buffered_channel<int>& chan_in, boost::fibers::buffered_channel<T>& toClose, size_t n)
    {
        int counter = 0;
        for(int increment : chan_in)
        {
            counter++;
            if(counter == n)
            {
                toClose.close();
                chan_in.close();
                return;
            }
        }
    }

    template <typename T>
    T RunAsync(vector<function<T()>> tasks, const std::function<int(const T&, const T&)>& compare)
    {
        //cfg
        const size_t num_generators = 1;
        const size_t num_workers = 2;
        const size_t num_mergers = 1;

        //define the channels
        size_t chanSize = pow(2, 4);
        boost::fibers::buffered_channel<function<T()>> workload_chan{chanSize};
        boost::fibers::buffered_channel<int> workload_closer{chanSize};
        
        boost::fibers::buffered_channel<T> worker_chan{chanSize};
        boost::fibers::buffered_channel<int> worker_closer{chanSize};
        
        boost::fibers::buffered_channel<T> merger_chan{chanSize};
        boost::fibers::buffered_channel<int> merger_closer{chanSize};
        
        boost::fibers::buffered_channel<T> finalMerger_chan{chanSize};
        boost::fibers::buffered_channel<int> finalMerger_closer{chanSize};

        //launch all the components

        //launch workload generator
        auto workloadGenerator = async([&tasks, &workload_chan](){
            for(auto& task : tasks)
            {
                workload_chan.push(task);
            }
            workload_chan.close();
        });

        //launch workers
        vector<future<void>> workers;
        for(int i = 0; i < num_workers; i++)
        {
            workers.push_back(async([&workload_chan, &worker_chan, &worker_closer](){
                Worker(workload_chan, worker_chan, worker_closer);
            }));
        }

        auto workerCloser = async([&worker_closer, &worker_chan, num_workers](){
            Closer(worker_closer, worker_chan, num_workers);
        });

        //launch mergers
        vector<future<void>> mergers;
        for(int i = 0; i < num_mergers; i++)
        {
            mergers.push_back(async([&compare, &worker_chan, &merger_chan, &merger_closer](){
                Merger(compare, worker_chan, merger_chan, merger_closer);
            }));
        }

        auto mergerCloser = async([&merger_closer, &merger_chan, num_mergers](){
            Closer(merger_closer, merger_chan, num_mergers);
        });

        //launch the final merger
        auto finalMerger = async([&compare, &merger_chan, &finalMerger_chan, &finalMerger_closer](){
            Merger(compare, merger_chan, finalMerger_chan, finalMerger_closer);
        });

        finalMerger.wait();
        return finalMerger_chan.value_pop();
    }
}