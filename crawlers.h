#include <string>
#include <vector>
#include <unordered_set>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>

class Crawler
{
public:
    virtual ~Crawler() = default;

    virtual void seed_url(std::string seed) = 0;
    virtual void seed_url(std::vector<std::string> seeds) = 0;
    virtual void crawl() = 0;
};

class linearCrawler : public Crawler
{
public:
    void seed_url(std::string seed) override;
    void seed_url(std::vector<std::string> seeds) override;
    void crawl() override;

private:
    std::vector<std::string> pending_urls;
    std::unordered_set<std::string> seen_urls;
};

class MasterSlaveCrawler : public Crawler
{
public:
    void seed_url(std::string seed) override;
    void seed_url(std::vector<std::string> seeds) override;
    void crawl() override;

    struct SlaveMachinery
    {
        std::queue<std::string> url_queue;
        std::mutex url_queue_lock;
        std::atomic<bool> busy_signal{false};
    };

private:
    std::queue<std::string> pending_urls;
    std::unordered_set<std::string> seen_urls;
    std::mutex seen_lock;
    std::mutex pending_lock;
    void slave(SlaveMachinery *sm);
    void timing_print_thread();
};

class PureSlaveCrawler : public Crawler
{
public:
    void seed_url(std::string seed) override;
    void seed_url(std::vector<std::string> seeds) override;
    void crawl() override;

private:
    std::queue<std::string> pending_urls;
    std::unordered_set<std::string> seen_urls;
    std::mutex seen_lock;
    std::mutex pending_lock;
    void slave();
    void timing_print_thread();
};