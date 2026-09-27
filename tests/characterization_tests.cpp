#include "Logger.h"
#include "SistemaFicheiros.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

namespace fs = std::filesystem;

class TemporaryFixture {
public:
  TemporaryFixture() {
    const auto uniqueId =
        std::chrono::high_resolution_clock::now().time_since_epoch().count();
    root = fs::temp_directory_path() /
           ("file_system_manager_tests_" + std::to_string(uniqueId));

    fs::create_directories(root / "documents");
    fs::create_directories(root / "empty");
    writeFile(root / "small.txt", "abc");
    writeFile(root / "documents" / "largest.bin", "123456789");
  }

  ~TemporaryFixture() {
    std::error_code error;
    fs::remove_all(root, error);
  }

  TemporaryFixture(const TemporaryFixture &) = delete;
  TemporaryFixture &operator=(const TemporaryFixture &) = delete;

  fs::path root;

private:
  static void writeFile(const fs::path &path, const std::string &contents) {
    std::ofstream output(path, std::ios::binary);
    output << contents;
    if (!output) {
      throw std::runtime_error("Could not create test fixture: " +
                               path.string());
    }
  }
};

void expect(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void testLoadAndStatistics() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;

  expect(fileSystem.Load(fixture.root.string()),
         "A valid directory should load successfully");
  expect(fileSystem.ContarFicheiros() == 2,
         "The fixture should contain two files");
  expect(fileSystem.ContarDirectorias() == 3,
         "Directory count should include the root and two children");
  expect(fileSystem.Memoria() > 0,
         "The current memory metric should be positive for loaded files");
}

void testInvalidDirectoryIsRejected() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  const fs::path missing = fixture.root / "does-not-exist";

  expect(!fileSystem.Load(missing.string()),
         "A missing directory should be rejected");
  expect(fileSystem.ContarFicheiros() == 0,
         "A rejected initial load should leave the system empty");
}

void testSearchAndLargestFile() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  std::string *filePath = fileSystem.Search("largest.bin", 0);
  expect(filePath != nullptr, "The nested file should be found");
  expect(fs::path(*filePath).filename() == "largest.bin",
         "File search should return the complete matching path");
  delete filePath;

  std::string *directoryPath = fileSystem.Search("documents", 1);
  expect(directoryPath != nullptr, "The nested directory should be found");
  expect(fs::path(*directoryPath).filename() == "documents",
         "Directory search should return the complete matching path");
  delete directoryPath;

  expect(fileSystem.Search("missing.txt", 0) == nullptr,
         "A missing file should not produce a result");

  std::string *largest = fileSystem.FicheiroMaior();
  expect(largest != nullptr, "A largest file should be returned");
  expect(fs::path(*largest).filename() == "largest.bin",
         "The largest fixture file should be selected");
  delete largest;
}

void testTreeOutput() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  const fs::path treePath = fixture.root / "tree-output.txt";
  const std::string treePathString = treePath.string();
  fileSystem.Tree(&treePathString);

  std::ifstream input(treePath);
  const std::string tree((std::istreambuf_iterator<char>(input)),
                         std::istreambuf_iterator<char>());
  expect(tree.find("<D>") != std::string::npos,
         "Tree output should contain directory markers");
  expect(tree.find("<F> largest.bin") != std::string::npos,
         "Tree output should contain nested files");
}

void testXmlRoundTrip() {
  TemporaryFixture fixture;
  SistemaFicheiros original;
  expect(original.Load(fixture.root.string()), "Fixture load failed");

  const fs::path xmlPath = fixture.root / "snapshot.xml";
  original.Escrever_XML(xmlPath.string());
  expect(fs::exists(xmlPath), "XML export should create a file");

  SistemaFicheiros restored;
  expect(restored.Ler_XML(xmlPath.string()),
         "The exported XML should be imported successfully");
  expect(restored.ContarFicheiros() == original.ContarFicheiros(),
         "XML round-trip should preserve the file count");
  expect(restored.ContarDirectorias() == original.ContarDirectorias(),
         "XML round-trip should preserve the directory count");
}

} // namespace

int main() {
  const fs::path logPath = fs::temp_directory_path() /
                           "file_system_manager_characterization_tests.log";
  Logger::init(logPath.string());

  const std::vector<std::pair<std::string, std::function<void()>>> tests = {
      {"load and statistics", testLoadAndStatistics},
      {"invalid directory", testInvalidDirectoryIsRejected},
      {"search and largest file", testSearchAndLargestFile},
      {"tree output", testTreeOutput},
      {"XML round-trip", testXmlRoundTrip},
  };

  std::size_t failures = 0;
  for (const auto &[name, test] : tests) {
    try {
      test();
      std::cout << "[PASS] " << name << '\n';
    } catch (const std::exception &error) {
      ++failures;
      std::cerr << "[FAIL] " << name << ": " << error.what() << '\n';
    }
  }

  if (failures != 0) {
    std::cerr << failures << " characterization test(s) failed\n";
    return 1;
  }

  std::cout << tests.size() << " characterization tests passed\n";
  return 0;
}
