#include <iostream>
#include "crawlers.h"

std::vector<std::string> seeds = {
    "www.npshsr.com",
    "www.wikipedia.org",
    "www.reddit.com",
    "www.github.com",
    "www.stackoverflow.com",
    "www.bbc.com",
    "www.cnn.com",
    "www.nytimes.com",
    "www.medium.com",
    "www.dev.to",
    "www.hackernews.com",
    "www.reuters.com",
    "www.apache.org",
    "www.mozilla.org",
    "www.rust-lang.org",
    "www.python.org"
};

void linear_crawler_test()
{
    Crawler *crawler = new linearCrawler();
    crawler->seed_url(seeds);
    crawler->crawl();
}

void master_slave_crawler_test()
{
    Crawler *crawler = new MasterSlaveCrawler();
    crawler->seed_url(seeds);
    crawler->crawl();
}

void pure_slave_crawler_test()
{
    Crawler *crawler = new PureSlaveCrawler();
    crawler->seed_url(seeds);
    crawler->crawl();
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cerr << "Usage: ./webCrawler --linear | --master-slave | --pure-slave\n";
        return 1;
    }

    std::string mode = argv[1];
    if (mode == "--linear")
        linear_crawler_test();
    else if (mode == "--master-slave")
        master_slave_crawler_test();
    else if (mode == "--pure-slave")
        pure_slave_crawler_test();
    else
    {
        std::cerr << "Unknown mode: " << mode << "\n";
        std::cerr << "Usage: ./webCrawler --linear | --master-slave | --pure-slave\n";
        return 1;
    }

    return 0;
}