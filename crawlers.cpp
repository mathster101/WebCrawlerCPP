#include "crawlers.h"
#include "utils.h"
#include <iostream>
#include <chrono>
#include <random>

#define NUM_THREADS 32
#define PRINT_URLS 0

using namespace std::chrono;

void linearCrawler::seed_url(std::string seed)
{
    pending_urls.push_back(normalize_url(seed));
}

void linearCrawler::seed_url(std::vector<std::string> seeds)
{
    for (auto url : seeds)
    {
        pending_urls.push_back(normalize_url(url));
    }
}

void linearCrawler::crawl()
{
    if (pending_urls.empty())
        return;
    steady_clock::time_point prev_time = steady_clock::now();
    long prev_size = 0;
    do
    {
        std::string current_url = pending_urls[0];
        seen_urls.insert(current_url);
        pending_urls.erase(pending_urls.begin());
#if PRINT_URLS
        std::cout << "going to crawl from " << current_url << "\n";
#endif

        std::string page_html = fetch_url(current_url);

        std::vector<std::string> urls = extract_urls_from_html(current_url, page_html);

        for (auto &url : urls)
        {
            if (seen_urls.find(url) == seen_urls.end())
                pending_urls.push_back(url);
        }
        steady_clock::time_point now_time = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now_time - prev_time);
        if (elapsed.count() > 5000)
        {
            long now_size = seen_urls.size();
            long pending_size = pending_urls.size();
            std::cout << (now_size - prev_size) / 5 << " urls/second | " << now_size << " urls seen | " << pending_size << " pending\n";
            prev_time = now_time;
            prev_size = now_size;
        }

    } while (!pending_urls.empty());
}

///////////////////////////////////////////////////////////////////
void MasterSlaveCrawler::seed_url(std::string seed)
{
    pending_urls.push(normalize_url(seed));
}

void MasterSlaveCrawler::seed_url(std::vector<std::string> seeds)
{
    for (auto url : seeds)
    {
        pending_urls.push(normalize_url(url));
    }
}

void MasterSlaveCrawler::slave(SlaveMachinery *sm)
{
    int timeout_counter = 0;
    while (timeout_counter < 1000)
    {
        sm->url_queue_lock.lock();
        if (sm->url_queue.empty())
        {
            sm->url_queue_lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            timeout_counter++;
            sm->busy_signal = false;
            continue;
        }
        timeout_counter = 0;
        sm->busy_signal = true;
        std::string current_url = sm->url_queue.front();
        sm->url_queue.pop();
        sm->url_queue_lock.unlock();

        seen_lock.lock();
        if (seen_urls.find(current_url) != seen_urls.end())
        {
            seen_lock.unlock();
            sm->busy_signal = false;
            continue;
        }
        seen_urls.insert(current_url);
        seen_lock.unlock();
#if PRINT_URLS
        std::cout << "going to crawl from " << current_url << std::endl;
#endif
        std::string page_html = fetch_url(current_url);
        std::vector<std::string> urls = extract_urls_from_html(current_url, page_html);
        pending_lock.lock();
        for (auto url : urls)
        {
            pending_urls.push(url);
        }
        pending_lock.unlock();
        sm->busy_signal = false;
    }
    std::cout << "thread timed out!!\n";
    delete sm;
}

void MasterSlaveCrawler::crawl()
{
    if (pending_urls.empty())
        return;

    std::vector<SlaveMachinery *> machinery_vector;
    std::vector<std::thread> threads;

    for (int i = 0; i < NUM_THREADS; i++)
    {
        SlaveMachinery *sm = new SlaveMachinery();
        std::thread t(&MasterSlaveCrawler::slave, this, sm);
        threads.push_back(std::move(t));
        machinery_vector.push_back(sm);
    }

    std::thread _timing_print_thread(&MasterSlaveCrawler::timing_print_thread, this);
    _timing_print_thread.detach();

    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<int> dist(0, NUM_THREADS - 1);

    int master_timeout = 0;
    while (master_timeout < 1000)
    {
        pending_lock.lock();
        int num_pending_urls = pending_urls.size();
        pending_lock.unlock();
        if (num_pending_urls == 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            master_timeout++;
            continue;
        }
        master_timeout = 0;
        for (int i = 0; i < std::min(NUM_THREADS, num_pending_urls); i++)
        {
            pending_lock.lock();
            std::string url = pending_urls.front();
            pending_urls.pop();
            pending_lock.unlock();

            while (true)
            {
                int chosen_thread = dist(rng);
                if (machinery_vector[chosen_thread]->busy_signal == false)
                {
                    machinery_vector[chosen_thread]->url_queue_lock.lock();
                    machinery_vector[chosen_thread]->url_queue.push(url);
                    machinery_vector[chosen_thread]->url_queue_lock.unlock();
                    break;
                }
                else
                    continue;
            }
        }
    }

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }
}

void MasterSlaveCrawler::timing_print_thread()
{
    steady_clock::time_point prev_time = steady_clock::now();
    long prev_size = 0;
    while (true)
    {
        steady_clock::time_point now_time = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now_time - prev_time);
        if (elapsed.count() > 5000)
        {
            seen_lock.lock();
            long now_size = seen_urls.size();
            seen_lock.unlock();
            pending_lock.lock();
            long pending_size = pending_urls.size();
            pending_lock.unlock();
            std::cout << (now_size - prev_size) / 5 << " urls/second | " << now_size << " urls seen | " << pending_size << " pending\n";
            prev_time = now_time;
            prev_size = now_size;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
///////////////////////////////////////////////////////////////////
void PureSlaveCrawler::seed_url(std::string seed)
{
    pending_urls.push(normalize_url(seed));
}

void PureSlaveCrawler::seed_url(std::vector<std::string> seeds)
{
    for (auto url : seeds)
    {
        pending_urls.push(normalize_url(url));
    }
}

void PureSlaveCrawler::slave()
{
    int timeout_counter = 0;
    while (timeout_counter < 1000)
    {
        pending_lock.lock();
        if (pending_urls.size() == 0)
        {
            pending_lock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            timeout_counter++;
            continue;
        }
        timeout_counter = 0;
        std::string current_url = pending_urls.front();
        pending_urls.pop();
        pending_lock.unlock();

        seen_lock.lock();
        if (seen_urls.find(current_url) != seen_urls.end())
        {
            seen_lock.unlock();
            continue;
        }

        seen_urls.insert(current_url);
        seen_lock.unlock();
#if PRINT_URLS
        std::cout << "processing " << current_url << std::endl;
#endif
        std::string page_html = fetch_url(current_url);
        std::vector<std::string> urls = extract_urls_from_html(current_url, page_html);
        pending_lock.lock();
        for (auto url : urls)
        {
            pending_urls.push(url);
        }
        pending_lock.unlock();
    }
    std::cout << "thread timed out!!\n";
}

void PureSlaveCrawler::timing_print_thread()
{
    steady_clock::time_point prev_time = steady_clock::now();
    long prev_size = 0;
    while (true)
    {
        steady_clock::time_point now_time = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now_time - prev_time);
        if (elapsed.count() > 5000)
        {
            seen_lock.lock();
            long now_size = seen_urls.size();
            seen_lock.unlock();
            pending_lock.lock();
            long pending_size = pending_urls.size();
            pending_lock.unlock();
            std::cout << (now_size - prev_size) / 5 << " urls/second | " << now_size << " urls seen | " << pending_size << " pending\n";
            prev_time = now_time;
            prev_size = now_size;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void PureSlaveCrawler::crawl()
{
    std::vector<std::thread> threads;
    for (int i = 0; i < NUM_THREADS; i++)
    {
        std::thread t(&PureSlaveCrawler::slave, this);
        threads.push_back(std::move(t));
    }

    std::thread _timing_print_thread(&PureSlaveCrawler::timing_print_thread, this);
    _timing_print_thread.detach();

    for (int i = 0; i < NUM_THREADS; i++)
    {
        threads[i].join();
    }
}