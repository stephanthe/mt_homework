#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <vector>

const size_t TOPK = 10;

using Counter = std::map<std::string, std::size_t>;

std::string tolower(const std::string& str);
void count_words(std::istream& stream, Counter&);
void print_topk(std::ostream& stream, const Counter&, const size_t k);
Counter process_file(const std::string& filename);

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: topk_words [FILES...]\n";
    return EXIT_FAILURE;
  }

  auto start = std::chrono::high_resolution_clock::now();

  Counter freq_dict;

  std::vector<std::future<Counter>> futures;

  for (int i = 1; i < argc; ++i) {
    futures.emplace_back(std::async(std::launch::async, process_file, argv[i]));
  }

  for (auto& f : futures) {
    auto partial_file = f.get();

    for (const auto& pair : partial_file) {
      freq_dict[pair.first] += pair.second;
    }
  }

  print_topk(std::cout, freq_dict, TOPK);

  auto end = std::chrono::high_resolution_clock::now();
  auto elapsed_ms =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  std::cout << "Elapsed time is " << elapsed_ms.count() << " us\n";
  freq_dict.clear();
}

Counter process_file(const std::string& filename) {
  std::ifstream input{filename};
  if (!input.is_open()) {
    std::cerr << "Failed to open file " << filename << '\n';
    return {};
  }
  Counter local_counter;
  count_words(input, local_counter);
  return local_counter;
}

std::string tolower(const std::string& str) {
  std::string lower_str;
  std::transform(std::cbegin(str), std::cend(str),
                 std::back_inserter(lower_str),
                 [](unsigned char ch) { return std::tolower(ch); });
  return lower_str;
};

void count_words(std::istream& stream, Counter& counter) {
  std::for_each(std::istream_iterator<std::string>(stream),
                std::istream_iterator<std::string>(),
                [&counter](const std::string& s) { ++counter[tolower(s)]; });
}

void print_topk(std::ostream& stream, const Counter& counter, const size_t k) {
  std::vector<Counter::const_iterator> words;
  words.reserve(counter.size());
  for (auto it = std::cbegin(counter); it != std::cend(counter); ++it) {
    words.push_back(it);
  }

  std::partial_sort(
      std::begin(words), std::begin(words) + k, std::end(words),
      [](auto lhs, auto& rhs) { return lhs->second > rhs->second; });

  std::for_each(std::begin(words), std::begin(words) + k,
                [&stream](const Counter::const_iterator& pair) {
                  stream << std::setw(4) << pair->second << " " << pair->first
                         << '\n';
                });
}