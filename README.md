# WebCrawlerCPP

Multi-threaded web crawler in C++11.

## Dependencies

```bash
sudo apt install libcurl4-openssl-dev libgumbo-dev
```

- **libcurl** — HTTP fetching
- **libgumbo** — HTML parsing and URL extraction

## Build

```bash
make
```

## Usage

```bash
./webCrawler --linear
./webCrawler --master-slave
./webCrawler --pure-slave
```

## Crawlers

- **linearCrawler** — Single-threaded. Processes one URL at a time from a queue. Baseline for comparison.

- **MasterSlaveCrawler** — A master thread distributes URLs from a shared pending queue to per-thread queues, randomly selecting idle slaves via a busy signal. Slaves pull from their own queue and self-terminate after 5s idle.

- **PureSlaveCrawler** — All threads pull directly from a single shared pending queue. Simpler design, but higher lock contention on the shared queue under load.
