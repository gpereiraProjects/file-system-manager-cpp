#include "Diretoria.h"
#include "Ficheiro.h"
#include "Item.h"
#include "Logger.h"
#include "Menu.h"
#include "SistemaFicheiros.h"
#include "Utils.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

static_assert(std::is_abstract_v<Item>,
              "Item must remain an abstract domain base class");
static_assert(std::has_virtual_destructor_v<Item>,
              "Items must be safely destructible through the base type");
static_assert(std::is_base_of_v<Item, Ficheiro>);
static_assert(std::is_base_of_v<Item, Diretoria>);
static_assert(std::is_same_v<
              decltype(std::declval<const SistemaFicheiros &>().Search(
                  std::declval<const std::string &>(),
                  SistemaFicheiros::TipoItem::Ficheiro)),
              std::optional<std::string>>,
              "Search results must use value semantics");
static_assert(std::is_same_v<
              decltype(std::declval<const SistemaFicheiros &>()
                           .PesquisarAllFicheiros(
                               std::declval<const std::string &>())),
              std::list<std::string>>,
              "Collection queries must return owned values");
static_assert(std::is_same_v<
              decltype(std::declval<const SistemaFicheiros &>()
                           .ContarFicheiros()),
              std::size_t>,
              "File counts must use std::size_t");
static_assert(std::is_same_v<
              decltype(std::declval<const SistemaFicheiros &>().Memoria()),
              std::uintmax_t>,
              "Byte totals must use std::uintmax_t");

namespace {

namespace fs = std::filesystem;
using TipoItem = SistemaFicheiros::TipoItem;

class ScopedConsoleInput {
public:
  explicit ScopedConsoleInput(const std::string &contents) : input(contents) {
    previousInput = std::cin.rdbuf(input.rdbuf());
    previousOutput = std::cout.rdbuf(output.rdbuf());
    std::cin.clear();
    std::cout.clear();
  }

  ~ScopedConsoleInput() {
    std::cin.rdbuf(previousInput);
    std::cout.rdbuf(previousOutput);
    std::cin.clear();
    std::cout.clear();
  }

  ScopedConsoleInput(const ScopedConsoleInput &) = delete;
  ScopedConsoleInput &operator=(const ScopedConsoleInput &) = delete;

  std::string capturedOutput() const { return output.str(); }

private:
  std::istringstream input;
  std::ostringstream output;
  std::streambuf *previousInput = nullptr;
  std::streambuf *previousOutput = nullptr;
};

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

template <typename T>
void expect(const std::optional<T> &condition, const std::string &message) {
  expect(condition.has_value(), message);
}

bool createDirectorySymlinkForTest(const fs::path &target,
                                   const fs::path &link,
                                   std::error_code &error) {
#ifdef _WIN32
  constexpr DWORD allowUnprivilegedCreate = 0x2;
  if (CreateSymbolicLinkW(link.c_str(), target.c_str(),
                          SYMBOLIC_LINK_FLAG_DIRECTORY |
                              allowUnprivilegedCreate) != 0) {
    error.clear();
    return true;
  }

  DWORD nativeError = GetLastError();
  if (nativeError == ERROR_INVALID_PARAMETER &&
      CreateSymbolicLinkW(link.c_str(), target.c_str(),
                          SYMBOLIC_LINK_FLAG_DIRECTORY) != 0) {
    error.clear();
    return true;
  }
  nativeError = GetLastError();
  error = std::error_code(static_cast<int>(nativeError),
                          std::system_category());
  return false;
#else
  fs::create_directory_symlink(target, link, error);
  return !error;
#endif
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

  std::optional<std::string> mostItems(
      fileSystem.DirectoriaMaisElementos());
  expect(mostItems && *mostItems == fixture.root.filename().string(),
         "The root should contain the greatest number of direct items");

  std::optional<std::string> fewestItems(
      fileSystem.DirectoriaMenosElementos());
  expect(fewestItems && *fewestItems == "empty",
         "The empty directory should contain the fewest items");

  std::optional<std::string> mostSpace(fileSystem.DirectoriaMaisEspaco());
  expect(mostSpace && fs::path(*mostSpace).filename() == "documents",
         "The documents directory should occupy the most space");
}

void testConstQueriesPreserveWideSizes() {
  TemporaryFixture fixture;
  const fs::path xmlPath = fixture.root / "wide-size.xml";
  constexpr std::uintmax_t expectedSize = UINTMAX_C(5000000000);
  {
    std::ofstream output(xmlPath, std::ios::binary);
    output << "<diretoria nome=\"root\" tamanho=\"" << expectedSize
           << "\"><ficheiro nome=\"large.bin\" tamanho=\"" << expectedSize
           << "\" extensao=\"bin\" dataModificacao=\"2026|09|29\"/>"
              "</diretoria>";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Ler_XML(xmlPath.string()),
         "The wide-size XML fixture should load");
  const SistemaFicheiros &view = fileSystem;

  expect(view.ContarFicheiros() == std::size_t{1},
         "Const queries should expose the exact file count");
  expect(view.ContarDirectorias() == std::size_t{1},
         "Const queries should expose the exact directory count");
  expect(view.Memoria() == expectedSize,
         "Byte totals must not be truncated to a 32-bit int");
  expect(view.Search("large.bin", TipoItem::Ficheiro),
         "Search should be callable through a const reference");
  expect(view.DataFicheiro("large.bin"),
         "Metadata lookup should be callable through a const reference");
  expect(view.PesquisarAllFicheiros("large.bin").size() == std::size_t{1},
         "Collection queries should be callable through a const reference");
  expect(!view.FicheiroDuplicados(),
         "Duplicate detection should be callable through a const reference");
}

void testPortablePathNormalization() {
  const fs::path unnormalized =
      fs::path("parent") / "." / "child" / ".." / "file.txt";
  expect(Utils::NormalizarCaminho(unnormalized.string()) ==
             unnormalized.lexically_normal().generic_string(),
         "Path normalization should follow std::filesystem semantics");

#ifndef _WIN32
  expect(Utils::NormalizarCaminho("directory\\file.txt") ==
             "directory\\file.txt",
         "A backslash must remain a valid filename character on POSIX");
#endif
}

void testMenuInputAndEndOfFileHandling() {
  SistemaFicheiros fileSystem;

  {
    ScopedConsoleInput console("");
    expect(!Menu::MenuInicializacao(fileSystem),
           "Initialization should stop cleanly at end of input");
  }

  {
    ScopedConsoleInput console("1\n");
    expect(!Menu::MenuInicializacao(fileSystem),
           "EOF while reading an initial path should stop cleanly");
  }

  {
    ScopedConsoleInput console("1\nnot-a-number\n\n");
    expect(!Menu::ExecutarOpcao(fileSystem),
           "EOF inside a submenu should propagate to the main menu");
    expect(console.capturedOutput().find("Entrada inválida") !=
               std::string::npos,
           "Invalid numeric input should produce a clear error");
  }

  {
    ScopedConsoleInput console("2\n1\nitem.txt\n");
    expect(!Menu::ExecutarOpcao(fileSystem),
           "EOF while reading an item type should stop the menu");
  }

  {
    ScopedConsoleInput console("7 trailing-text\n\n7\n");
    expect(!Menu::ExecutarOpcao(fileSystem),
           "The explicit exit option should still stop the menu");
    expect(console.capturedOutput().find("Entrada inválida") !=
               std::string::npos,
           "A number followed by extra text must be rejected");
  }

  for (const char *menuOption : {"1\n", "2\n", "3\n", "4\n", "5\n",
                                 "6\n"}) {
    ScopedConsoleInput console(menuOption);
    expect(!Menu::ExecutarOpcao(fileSystem),
           "Every submenu should propagate end of input");
  }
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
  std::optional<std::string> preserved(
      fileSystem.Search("small.txt", TipoItem::Ficheiro));
  expect(preserved,
         "Previously loaded items should survive a rejected load");
}

void testSymbolicLinkCyclesAreIgnored() {
  TemporaryFixture fixture;
  const fs::path cycle = fixture.root / "documents" / "back-to-root";
  std::error_code linkError;
  if (!createDirectorySymlinkForTest(fixture.root, cycle, linkError)) {
    std::cout << "[INFO] symbolic-link test unavailable: "
              << linkError.message() << '\n';
    return;
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()),
         "A tree containing a symbolic-link cycle should load safely");
  expect(fileSystem.ContarDirectorias() == 3,
         "A symbolic directory link must not become part of the loaded tree");
  expect(fileSystem.ContarFicheiros() == 2,
         "A symbolic-link cycle must not duplicate files");
  expect(fileSystem.Memoria() == 12,
         "A symbolic-link cycle must not duplicate file sizes");
  std::optional<std::string> ignoredLink(
      fileSystem.Search("documents/back-to-root", TipoItem::Diretoria));
  expect(!ignoredLink,
         "An ignored symbolic link must not be searchable as a directory");
}

void testSearchAndLargestFile() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  std::optional<std::string> filePath(
      fileSystem.Search("documents/largest.bin", TipoItem::Ficheiro));
  expect(filePath, "The nested file should be found");
  expect(fs::path(*filePath).filename() == "largest.bin",
         "File search should return the complete matching path");

  std::optional<std::string> directoryPath(
      fileSystem.Search("documents", TipoItem::Diretoria));
  expect(directoryPath, "The nested directory should be found");
  expect(fs::path(*directoryPath).filename() == "documents",
         "Directory search should return the complete matching path");

  expect(!fileSystem.Search("missing.txt", TipoItem::Ficheiro),
         "A missing file should not produce a result");

  std::optional<std::string> largest(fileSystem.FicheiroMaior());
  expect(largest, "A largest file should be returned");
  expect(fs::path(*largest).filename() == "largest.bin",
         "The largest fixture file should be selected");
}

void testPathSearchSelectsExactItems() {
  TemporaryFixture fixture;
  {
    std::ofstream documentsCopy(fixture.root / "documents" / "small.txt",
                                std::ios::binary);
    documentsCopy << "documents";
    std::ofstream emptyCopy(fixture.root / "empty" / "small.txt",
                            std::ios::binary);
    emptyCopy << "empty";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  std::optional<std::string> rootFile(
      fileSystem.Search("small.txt", TipoItem::Ficheiro));
  std::optional<std::string> documentsFile(
      fileSystem.Search("documents/small.txt", TipoItem::Ficheiro));
  std::optional<std::string> emptyFile(
      fileSystem.Search("empty/small.txt", TipoItem::Ficheiro));
  expect(rootFile && documentsFile && emptyFile,
         "Relative paths should resolve every repeated filename exactly");
  expect(fs::path(*rootFile).parent_path() == fixture.root,
         "A leaf path should resolve only at the root level");
  expect(fs::path(*documentsFile).parent_path().filename() == "documents" &&
             fs::path(*emptyFile).parent_path().filename() == "empty",
         "Nested paths should select the requested parent directory");

  std::optional<std::string> absoluteFile(fileSystem.Search(
      (fixture.root / "documents" / "small.txt").string(),
      TipoItem::Ficheiro));
  expect(absoluteFile && *absoluteFile == *documentsFile,
         "An absolute path inside the loaded root should also resolve");
  expect(!fileSystem.Search("largest.bin", TipoItem::Ficheiro),
         "A bare name must not silently select a nested item");
  expect(!fileSystem.Search("../small.txt", TipoItem::Ficheiro),
         "A path must not escape the loaded root");
  expect(!fileSystem.Search("documents/small.txt", TipoItem::Diretoria),
         "Path search must enforce the requested item type");

  std::optional<std::string> modificationDate(
      fileSystem.DataFicheiro("documents/small.txt"));
  expect(modificationDate,
         "File metadata lookup should use the same exact path resolution");
  expect(!fileSystem.DataFicheiro("largest.bin"),
         "File metadata lookup must not fall back to recursive name search");
}

void testPathMovesSelectExactItems() {
  TemporaryFixture fixture;
  fs::create_directories(fixture.root / "documents" / "target");
  fs::create_directories(fixture.root / "empty" / "target");
  fs::create_directories(fixture.root / "documents" / "nested");
  fs::create_directories(fixture.root / "empty" / "nested");
  {
    std::ofstream documentsFile(fixture.root / "documents" / "same.txt",
                                std::ios::binary);
    documentsFile << "documents";
    std::ofstream emptyFile(fixture.root / "empty" / "same.txt",
                            std::ios::binary);
    emptyFile << "empty";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  expect(!fileSystem.MoveFicheiro("same.txt", "empty/target"),
         "A bare nested filename must not move an arbitrary match");
  expect(fileSystem.MoveFicheiro("documents/same.txt", "empty/target"),
         "A complete relative path should move the selected file");
  expect(!fs::exists(fixture.root / "documents" / "same.txt") &&
             fs::exists(fixture.root / "empty" / "same.txt") &&
             fs::exists(fixture.root / "empty" / "target" / "same.txt"),
         "Only the file identified by its path should move");

  expect(!fileSystem.MoverDirectoria("nested", "."),
         "A bare nested directory name must not move an arbitrary match");
  expect(fileSystem.MoverDirectoria("documents/nested", "."),
         "A complete relative path should move the selected directory");
  expect(fs::exists(fixture.root / "nested") &&
             fs::exists(fixture.root / "empty" / "nested") &&
             !fs::exists(fixture.root / "documents" / "nested"),
         "Only the directory identified by its path should move");
}

void testTreeOutput() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  const fs::path treePath = fixture.root / "tree-output.txt";
  const std::string treePathString = treePath.string();
  fileSystem.Tree(treePathString);

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
  std::optional<std::string> specialFile(
      restored.Search("R&D's/notes & ideas.txt", TipoItem::Ficheiro));
  expect(specialFile,
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
  std::optional<std::string> decoded(
      fileSystem.Search("café.txt", TipoItem::Ficheiro));
  expect(decoded,
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
      "<diretoria nome=\"root\" tamanho=\"1\"><ficheiro nome=\"CON.txt\" "
      "tamanho=\"1\" extensao=\"txt\" "
      "dataModificacao=\"\"/></diretoria>",
      "<diretoria nome=\"root\" tamanho=\"2\"><ficheiro nome=\"same.txt\" "
      "tamanho=\"1\" extensao=\"txt\" dataModificacao=\"\"/><ficheiro "
      "nome=\"SAME.TXT\" tamanho=\"1\" extensao=\"txt\" "
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
    std::optional<std::string> original(
        fileSystem.Search("small.txt", TipoItem::Ficheiro));
    expect(original,
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

  std::optional<std::string> movedFile(
      fileSystem.Search("documents/small.txt", TipoItem::Ficheiro));
  expect(movedFile, "The moved file should remain searchable");
  expect(fs::path(*movedFile).parent_path().filename() == "documents",
         "The moved file path should reference its new parent");

  expect(fileSystem.MoverDirectoria("documents", "empty"),
         "Moving a directory should transfer its complete subtree");
  expect(fileSystem.ContarDirectorias() == 3,
         "Moving a directory should preserve the directory count");
  expect(fileSystem.ContarFicheiros() == 2,
         "Moving a directory should preserve all contained files");

  std::optional<std::string> movedDirectory(
      fileSystem.Search("empty/documents", TipoItem::Diretoria));
  expect(movedDirectory,
         "The moved directory should remain searchable");
  expect(fs::path(*movedDirectory).parent_path().filename() == "empty",
         "The moved directory path should reference its new parent");

  std::optional<std::string> largestDirectory(
      fileSystem.DirectoriaMaisEspaco());
  expect(largestDirectory,
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
  std::optional<std::string> original(
      fileSystem.Search("small.txt", TipoItem::Ficheiro));
  expect(original,
         "Original data should remain available after invalid XML");
}

void testCyclicDirectoryMoveIsRejected() {
  TemporaryFixture fixture;
  fs::create_directories(fixture.root / "documents" / "nested");

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
  expect(!fileSystem.MoverDirectoria("documents", "documents/nested"),
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

  expect(fileSystem.RemoverAll("small.txt", TipoItem::Ficheiro),
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
  std::optional<std::string> renamed(
      fileSystem.Search("renamed.txt", TipoItem::Ficheiro));
  expect(renamed,
         "The renamed file should be represented in the in-memory tree");
  expect(!fileSystem.Search("small.txt", TipoItem::Ficheiro),
         "The old filename should no longer be represented in memory");
}

void testRenameUpdatesExtensionMetadata() {
  TemporaryFixture fixture;
  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  fileSystem.RenomearFicheiros("small.txt", "renamed.bin");
  expect(fs::exists(fixture.root / "renamed.bin"),
         "Renaming to a different extension should update the disk");

  const fs::path xmlPath = fixture.root / "renamed.xml";
  expect(fileSystem.Escrever_XML(xmlPath.string()),
         "The renamed tree should remain exportable");
  std::ifstream input(xmlPath, std::ios::binary);
  const std::string xml((std::istreambuf_iterator<char>(input)),
                        std::istreambuf_iterator<char>());
  expect(xml.find("nome=\"renamed.bin\"") != std::string::npos,
         "XML should contain the new filename");
  expect(xml.find("nome=\"renamed.bin\" tamanho=\"3\" extensao=\"bin\"") !=
             std::string::npos,
         "The extension metadata should follow the renamed file");
}

void testRenameRejectsInvalidNamesAndCollisions() {
  TemporaryFixture fixture;
  {
    std::ofstream collision(fixture.root / "renamed.txt", std::ios::binary);
    collision << "occupied";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");

  const std::vector<std::string> invalidNames = {
      "",          ".",           "..",        "../escaped.txt",
      "sub/name",  "sub\\name",  "CON.txt",   "bad?.txt",
      "trailing.", "trailing ",
      (fixture.root / "absolute.txt").string()};

  for (const std::string &invalidName : invalidNames) {
    fileSystem.RenomearFicheiros("small.txt", invalidName);
    expect(fs::exists(fixture.root / "small.txt"),
           "An invalid target name must preserve the source file");
    std::optional<std::string> preserved(
        fileSystem.Search("small.txt", TipoItem::Ficheiro));
    expect(preserved,
           "An invalid target name must preserve the in-memory item");
  }

  fileSystem.RenomearFicheiros("small.txt", "renamed.txt");
  expect(fs::exists(fixture.root / "small.txt"),
         "A destination collision must preserve the source file");
  expect(fs::exists(fixture.root / "renamed.txt"),
         "A destination collision must preserve the existing file");
  expect(fileSystem.ContarFicheiros() == 3,
         "A rejected collision must preserve all in-memory files");
}

void testBatchRenameIsAllOrNothing() {
  {
    TemporaryFixture fixture;
    {
      std::ofstream rootMatch(fixture.root / "same.txt", std::ios::binary);
      rootMatch << "root";
      std::ofstream nestedMatch(fixture.root / "documents" / "same.txt",
                                std::ios::binary);
      nestedMatch << "nested";
      std::ofstream collision(fixture.root / "documents" / "target.txt",
                              std::ios::binary);
      collision << "occupied";
    }

    SistemaFicheiros fileSystem;
    expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
    fileSystem.RenomearFicheiros("same.txt", "target.txt");
    expect(fs::exists(fixture.root / "same.txt") &&
               fs::exists(fixture.root / "documents" / "same.txt"),
           "A collision in one directory must cancel the complete batch");
    expect(!fs::exists(fixture.root / "target.txt"),
           "A rejected batch must not rename earlier matches");
  }

  {
    TemporaryFixture fixture;
    {
      std::ofstream rootMatch(fixture.root / "same.txt", std::ios::binary);
      rootMatch << "root";
      std::ofstream nestedMatch(fixture.root / "documents" / "same.txt",
                                std::ios::binary);
      nestedMatch << "nested";
    }

    SistemaFicheiros fileSystem;
    expect(fileSystem.Load(fixture.root.string()), "Fixture load failed");
    fileSystem.RenomearFicheiros("same.txt", "renamed.dat");
    expect(fs::exists(fixture.root / "renamed.dat") &&
               fs::exists(fixture.root / "documents" / "renamed.dat"),
           "Every non-conflicting match should be renamed");
    expect(!fs::exists(fixture.root / "same.txt") &&
               !fs::exists(fixture.root / "documents" / "same.txt"),
           "A successful batch should remove every old path");
  }
}

void testXmlRenameRejectsPortableCollision() {
  TemporaryFixture fixture;
  const fs::path xmlPath = fixture.root / "virtual.xml";
  {
    std::ofstream output(xmlPath, std::ios::binary);
    output << "<diretoria nome=\"root\" tamanho=\"2\">"
              "<ficheiro nome=\"a.txt\" tamanho=\"1\" extensao=\"txt\" "
              "dataModificacao=\"\"/>"
              "<ficheiro nome=\"b.txt\" tamanho=\"1\" extensao=\"txt\" "
              "dataModificacao=\"\"/>"
              "</diretoria>";
  }

  SistemaFicheiros fileSystem;
  expect(fileSystem.Ler_XML(xmlPath.string()), "Valid virtual tree failed");
  fileSystem.RenomearFicheiros("a.txt", "B.TXT");
  std::optional<std::string> first(
      fileSystem.Search("a.txt", TipoItem::Ficheiro));
  std::optional<std::string> second(
      fileSystem.Search("b.txt", TipoItem::Ficheiro));
  expect(first && second,
         "A portable case-insensitive collision must preserve both items");
  expect(fileSystem.ContarFicheiros() == 2,
         "A rejected virtual rename must preserve the tree");
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
  expect(fileSystem.RemoverAll("documents", TipoItem::Diretoria),
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

  expect(!fileSystem.RemoverAll(fixture.root.filename().string(),
                                TipoItem::Diretoria),
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
      {"const queries and wide sizes", testConstQueriesPreserveWideSizes},
      {"portable path normalization", testPortablePathNormalization},
      {"menu input and EOF", testMenuInputAndEndOfFileHandling},
      {"invalid directory", testInvalidDirectoryIsRejected},
      {"symbolic-link cycle", testSymbolicLinkCyclesAreIgnored},
      {"search and largest file", testSearchAndLargestFile},
      {"exact path search", testPathSearchSelectsExactItems},
      {"exact path moves", testPathMovesSelectExactItems},
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
      {"rename extension metadata", testRenameUpdatesExtensionMetadata},
      {"rename validation and collisions",
       testRenameRejectsInvalidNamesAndCollisions},
      {"transactional batch rename", testBatchRenameIsAllOrNothing},
      {"virtual rename collision", testXmlRenameRejectsPortableCollision},
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
