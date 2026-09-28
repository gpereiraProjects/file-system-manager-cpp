#include "Diretoria.h"
#include "Ficheiro.h"
#include "Item.h"
#include "Logger.h"
#include "SistemaFicheiros.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

static_assert(std::is_abstract_v<Item>,
              "Item must remain an abstract domain base class");
static_assert(std::has_virtual_destructor_v<Item>,
              "Items must be safely destructible through the base type");
static_assert(std::is_base_of_v<Item, Ficheiro>);
static_assert(std::is_base_of_v<Item, Diretoria>);

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
  expect(fileSystem.Memoria() == 12,
         "Memory usage should equal the total file size in bytes");

  std::unique_ptr<std::string> mostItems(
      fileSystem.DirectoriaMaisElementos());
  expect(mostItems != nullptr && *mostItems == fixture.root.filename().string(),
         "The root should contain the greatest number of direct items");

  std::unique_ptr<std::string> fewestItems(
      fileSystem.DirectoriaMenosElementos());
  expect(fewestItems != nullptr && *fewestItems == "empty",
         "The empty directory should contain the fewest items");

  std::unique_ptr<std::string> mostSpace(fileSystem.DirectoriaMaisEspaco());
  expect(mostSpace != nullptr && fs::path(*mostSpace).filename() == "documents",
         "The documents directory should occupy the most space");
}

void testInvalidDirectoryIsRejected() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
  const fs::path missing = fixture.root / "does-not-exist";

  expect(!fileSystem.Load(missing.string()),
         "A missing directory should be rejected");
  expect(fileSystem.ContarFicheiros() == 2,
         "A rejected load should preserve the previous state");
  std::unique_ptr<std::string> preserved(fileSystem.Search("small.txt", 0));
  expect(preserved != nullptr,
         "Previously loaded items should survive a rejected load");
}

void testSearchAndLargestFile() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  std::unique_ptr<std::string> filePath(fileSystem.Search("largest.bin", 0));
  expect(filePath != nullptr, "The nested file should be found");
  expect(fs::path(*filePath).filename() == "largest.bin",
         "File search should return the complete matching path");

  std::unique_ptr<std::string> directoryPath(
      fileSystem.Search("documents", 1));
  expect(directoryPath != nullptr, "The nested directory should be found");
  expect(fs::path(*directoryPath).filename() == "documents",
         "Directory search should return the complete matching path");

  expect(fileSystem.Search("missing.txt", 0) == nullptr,
         "A missing file should not produce a result");

  std::unique_ptr<std::string> largest(fileSystem.FicheiroMaior());
  expect(largest != nullptr, "A largest file should be returned");
  expect(fs::path(*largest).filename() == "largest.bin",
         "The largest fixture file should be selected");
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
  expect(original.Escrever_XML(xmlPath.string()),
         "XML export should report success");
  expect(fs::exists(xmlPath), "XML export should create a file");

  SistemaFicheiros restored;
  expect(restored.Ler_XML(xmlPath.string()),
         "The exported XML should be imported successfully");
  expect(restored.ContarFicheiros() == original.ContarFicheiros(),
         "XML round-trip should preserve the file count");
  expect(restored.ContarDirectorias() == original.ContarDirectorias(),
         "XML round-trip should preserve the directory count");
}

void testXmlEscapesSpecialCharacters() {
  TemporaryFixture fixture;
  const fs::path specialDirectory = fixture.root / "R&D's";
  fs::create_directories(specialDirectory);
  {
    std::ofstream output(specialDirectory / "notes & ideas.txt",
                         std::ios::binary);
    output << "special";
  }

  SistemaFicheiros original;
  expect(original.Load(fixture.root.string()), "Fixture load failed");

  const fs::path xmlPath = fixture.root / "escaped.xml";
  expect(original.Escrever_XML(xmlPath.string()),
         "Special-character XML export should succeed");

  std::ifstream input(xmlPath, std::ios::binary);
  const std::string xml((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
  input.close();
  expect(xml.find("R&amp;D&apos;s") != std::string::npos,
         "Directory names must be escaped in XML attributes");
  expect(xml.find("notes &amp; ideas.txt") != std::string::npos,
         "File names must be escaped in XML attributes");

  SistemaFicheiros restored;
  expect(restored.Ler_XML(xmlPath.string()),
         "Escaped XML should import successfully");
  expect(restored.Memoria() == original.Memoria(),
         "XML round-trip should preserve file sizes");
  std::unique_ptr<std::string> specialFile(
      restored.Search("notes & ideas.txt", 0));
  expect(specialFile != nullptr,
         "Escaped file names should be decoded during import");

  expect(original.Escrever_XML(xmlPath.string()),
         "Replacing an existing XML snapshot should succeed");
  for (const auto &entry : fs::directory_iterator(fixture.root)) {
    const std::string name = entry.path().filename().string();
    expect(name.find("escaped.xml.tmp-") == std::string::npos &&
               name.find("escaped.xml.bak-") == std::string::npos,
           "Successful XML replacement must not leave temporary files");
  }
}

void testXmlAcceptsValidStructuralVariations() {
  TemporaryFixture fixture;
  const fs::path xmlPath = fixture.root / "structural.xml";
  {
    std::ofstream output(xmlPath, std::ios::binary);
    output << "<?xml version='1.0' encoding='UTF-8'?>\n"
              "<diretoria tamanho='3' nome='root'>\n"
              "  <ficheiro dataModificacao='' extensao='txt' tamanho='3' "
              "nome='caf&#xE9;.txt'/>\n"
              "</diretoria>\n";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Ler_XML(xmlPath.string()),
         "Valid attribute order, quotes and self-closing files should import");
  expect(fileSystem.Memoria() == 3,
         "Imported numeric metadata should be preserved");
  std::unique_ptr<std::string> decoded(fileSystem.Search("café.txt", 0));
  expect(decoded != nullptr,
         "Numeric XML character references should be decoded as UTF-8");
}

void testMalformedXmlVariantsPreserveState() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  const std::vector<std::string> invalidDocuments = {
      "<diretoria nome=\"root\" tamanho=\"99\"></diretoria>",
      "<diretoria nome=\"root\" nome=\"duplicate\" tamanho=\"0\">"
      "</diretoria>",
      "<diretoria nome=\"root\" tamanho=\"184467440737095516160\">"
      "</diretoria>",
      "<diretoria nome=\"root\" tamanho=\"1\"><ficheiro nome=\"../escape\" "
      "tamanho=\"1\" extensao=\"\" dataModificacao=\"\"/></diretoria>",
      "<diretoria nome=\"root\" tamanho=\"2\"><ficheiro nome=\"same\" "
      "tamanho=\"1\" extensao=\"\" dataModificacao=\"\"/><ficheiro "
      "nome=\"same\" tamanho=\"1\" extensao=\"\" "
      "dataModificacao=\"\"/></diretoria>",
      "<diretoria nome=\"root\" tamanho=\"0\"></diretoria>"
      "<diretoria nome=\"second\" tamanho=\"0\"></diretoria>",
      "<diretoria nome=\"root\" tamanho=\"0\" extra=\"unsupported\">"
      "</diretoria>",
      "<diretoria nome=\"bad&unknown;\" tamanho=\"0\"></diretoria>"};

  const fs::path xmlPath = fixture.root / "malformed.xml";
  for (const std::string &document : invalidDocuments) {
    {
      std::ofstream output(xmlPath, std::ios::binary | std::ios::trunc);
      output << document;
    }
    expect(!fileSystem.Ler_XML(xmlPath.string()),
           "Malformed XML variants must be rejected");
    expect(fileSystem.ContarFicheiros() == 2,
           "Rejected XML must preserve the active tree");
    std::unique_ptr<std::string> original(fileSystem.Search("small.txt", 0));
    expect(original != nullptr,
           "Original items must remain available after rejected XML");
  }
}

void testXmlExportRejectsDirectoryDestination() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
  expect(!fileSystem.Escrever_XML(fixture.root.string()),
         "XML export must reject a directory as its destination");
  expect(fs::exists(fixture.root / "small.txt"),
         "A rejected export must not alter the destination directory");
}

void testOwnershipTransfersRemainValid() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  expect(fileSystem.MoveFicheiro("small.txt", "documents"),
         "Moving a file should transfer it to the destination directory");
  expect(fileSystem.ContarFicheiros() == 2,
         "Moving a file should preserve the file count");

  std::unique_ptr<std::string> movedFile(fileSystem.Search("small.txt", 0));
  expect(movedFile != nullptr, "The moved file should remain searchable");
  expect(fs::path(*movedFile).parent_path().filename() == "documents",
         "The moved file path should reference its new parent");

  expect(fileSystem.MoverDirectoria("documents", "empty"),
         "Moving a directory should transfer its complete subtree");
  expect(fileSystem.ContarDirectorias() == 3,
         "Moving a directory should preserve the directory count");
  expect(fileSystem.ContarFicheiros() == 2,
         "Moving a directory should preserve all contained files");

  std::unique_ptr<std::string> movedDirectory(
      fileSystem.Search("documents", 1));
  expect(movedDirectory != nullptr,
         "The moved directory should remain searchable");
  expect(fs::path(*movedDirectory).parent_path().filename() == "empty",
         "The moved directory path should reference its new parent");

  std::unique_ptr<std::string> largestDirectory(
      fileSystem.DirectoriaMaisEspaco());
  expect(largestDirectory != nullptr,
         "The largest directory query should return a result");
  expect(fs::path(*largestDirectory).filename() == "empty",
         "Directory sizes should be recalculated after ownership transfers");
}

void testInvalidXmlPreservesState() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  const fs::path invalidXml = fixture.root / "invalid.xml";
  {
    std::ofstream output(invalidXml);
    output << "<?xml version=\"1.0\"?>\n"
              "<diretoria nome=\"broken\" tamanho=\"1\">\n"
              "  <ficheiro nome=\"partial.txt\" tamanho=\"1\" "
              "extensao=\"txt\" dataModificacao=\"2026|01|01\"></ficheiro>\n";
  }

  expect(!fileSystem.Ler_XML(invalidXml.string()),
         "An incomplete XML document should be rejected");
  expect(fileSystem.ContarFicheiros() == 2,
         "A rejected XML import should preserve the previous state");
  std::unique_ptr<std::string> original(fileSystem.Search("small.txt", 0));
  expect(original != nullptr,
         "Original data should remain available after invalid XML");
}

void testCyclicDirectoryMoveIsRejected() {
  TemporaryFixture fixture;
  fs::create_directories(fixture.root / "documents" / "nested");

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
  expect(!fileSystem.MoverDirectoria("documents", "nested"),
         "A directory must not be moved into its own subtree");
  expect(fileSystem.ContarDirectorias() == 4,
         "A rejected cyclic move should preserve the tree");
  expect(fs::exists(fixture.root / "documents" / "nested"),
         "A rejected cyclic move should preserve the physical directory");
}

void testRemovalUpdatesStatistics() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  expect(fileSystem.RemoverAll("small.txt", "FICH"),
         "An existing file should be removed from the in-memory tree");
  expect(fileSystem.ContarFicheiros() == 1,
         "Removal should update the file count");
  expect(fileSystem.Memoria() == 9,
         "Removal should update the total file size");
  expect(!fs::exists(fixture.root / "small.txt"),
         "Removal should keep the physical and in-memory trees consistent");
}

void testBatchCopyResolvesNameCollisions() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  expect(fileSystem.CopyBatch("largest", "documents", "empty"),
         "The first batch copy should succeed");
  expect(fs::exists(fixture.root / "empty" / "largest.bin"),
         "The first copied file should use its original name");

  expect(fileSystem.CopyBatch("largest", "documents", "empty"),
         "A repeated batch copy should resolve the name collision");
  expect(fs::exists(fixture.root / "empty" / "largest(001).bin"),
         "The repeated copy should use a sequential suffix");
  expect(fileSystem.CopyBatch("largest", "documents", "empty"),
         "A third batch copy should resolve all existing collisions");
  expect(fs::exists(fixture.root / "empty" / "largest(002).bin"),
         "Sequential suffixes should skip names that already exist");
  expect(fileSystem.ContarFicheiros() == 5,
         "All successful copies should be represented in memory");
  expect(fileSystem.Memoria() == 39,
         "Batch copies should update the total file size");
}

void testRenameKeepsDiskAndMemoryConsistent() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  fileSystem.RenomearFicheiros("small.txt", "renamed.txt");
  expect(!fs::exists(fixture.root / "small.txt"),
         "The original physical filename should no longer exist");
  expect(fs::exists(fixture.root / "renamed.txt"),
         "The renamed physical file should exist");
  std::unique_ptr<std::string> renamed(fileSystem.Search("renamed.txt", 0));
  expect(renamed != nullptr,
         "The renamed file should be represented in the in-memory tree");
  expect(fileSystem.Search("small.txt", 0) == nullptr,
         "The old filename should no longer be represented in memory");
}

void testRecursiveDirectoryRemoval() {
  TemporaryFixture fixture;
  fs::create_directories(fixture.root / "documents" / "nested");
  {
    std::ofstream output(fixture.root / "documents" / "nested" / "deep.txt");
    output << "deep";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
  expect(fileSystem.RemoverAll("documents", "DIR"),
         "An existing directory subtree should be removed");
  expect(!fs::exists(fixture.root / "documents"),
         "Recursive removal should update the physical file system");
  expect(fileSystem.ContarDirectorias() == 2,
         "Recursive removal should update the directory count");
  expect(fileSystem.ContarFicheiros() == 1,
         "Recursive removal should discard all files in the subtree");
  expect(fileSystem.Memoria() == 3,
         "Recursive removal should recalculate the total file size");
}

void testRootDirectoryCannotBeRemoved() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  expect(!fileSystem.RemoverAll(fixture.root.filename().string(), "DIR"),
         "The loaded root directory must not be removable");
  expect(fs::exists(fixture.root),
         "Rejecting root removal should preserve the physical directory");
  expect(fileSystem.ContarFicheiros() == 2,
         "Rejecting root removal should preserve the in-memory tree");
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
      {"XML special-character escaping", testXmlEscapesSpecialCharacters},
      {"XML structural variations", testXmlAcceptsValidStructuralVariations},
      {"malformed XML variants", testMalformedXmlVariantsPreserveState},
      {"XML directory destination", testXmlExportRejectsDirectoryDestination},
      {"ownership transfers", testOwnershipTransfersRemainValid},
      {"invalid XML transaction", testInvalidXmlPreservesState},
      {"cyclic directory move", testCyclicDirectoryMoveIsRejected},
      {"removal statistics", testRemovalUpdatesStatistics},
      {"batch copy collisions", testBatchCopyResolvesNameCollisions},
      {"rename consistency", testRenameKeepsDiskAndMemoryConsistent},
      {"recursive directory removal", testRecursiveDirectoryRemoval},
      {"root removal guard", testRootDirectoryCannotBeRemoved},
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
