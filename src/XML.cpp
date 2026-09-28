#include "XML.h"

#include "Diretoria.h"
#include "Ficheiro.h"
#include "Logger.h"
#include "Utils.h"

#include <charconv>
#include <cctype>
#include <iterator>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;

namespace {

string escapeXmlAttribute(const string &value) {
  string escaped;
  escaped.reserve(value.size());

  for (const char character : value) {
    switch (character) {
    case '&':
      escaped += "&amp;";
      break;
    case '<':
      escaped += "&lt;";
      break;
    case '>':
      escaped += "&gt;";
      break;
    case '"':
      escaped += "&quot;";
      break;
    case '\'':
      escaped += "&apos;";
      break;
    default:
      escaped += character;
      break;
    }
  }

  return escaped;
}

void appendUtf8(string &output, uint32_t codePoint) {
  if (codePoint == 0 || codePoint > 0x10FFFFU ||
      (codePoint >= 0xD800U && codePoint <= 0xDFFFU)) {
    throw runtime_error("Referência de carácter XML inválida.");
  }

  if (codePoint <= 0x7FU) {
    output += static_cast<char>(codePoint);
  } else if (codePoint <= 0x7FFU) {
    output += static_cast<char>(0xC0U | (codePoint >> 6U));
    output += static_cast<char>(0x80U | (codePoint & 0x3FU));
  } else if (codePoint <= 0xFFFFU) {
    output += static_cast<char>(0xE0U | (codePoint >> 12U));
    output += static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU));
    output += static_cast<char>(0x80U | (codePoint & 0x3FU));
  } else {
    output += static_cast<char>(0xF0U | (codePoint >> 18U));
    output += static_cast<char>(0x80U | ((codePoint >> 12U) & 0x3FU));
    output += static_cast<char>(0x80U | ((codePoint >> 6U) & 0x3FU));
    output += static_cast<char>(0x80U | (codePoint & 0x3FU));
  }
}

string decodeXmlAttribute(const string &value) {
  string decoded;
  decoded.reserve(value.size());

  for (size_t index = 0; index < value.size();) {
    if (value[index] != '&') {
      decoded += value[index++];
      continue;
    }

    const size_t end = value.find(';', index + 1);
    if (end == string::npos)
      throw runtime_error("Entidade XML sem terminador.");

    const string entity = value.substr(index + 1, end - index - 1);
    if (entity == "amp")
      decoded += '&';
    else if (entity == "lt")
      decoded += '<';
    else if (entity == "gt")
      decoded += '>';
    else if (entity == "quot")
      decoded += '"';
    else if (entity == "apos")
      decoded += '\'';
    else if (!entity.empty() && entity.front() == '#') {
      const bool hexadecimal = entity.size() > 1 &&
                               (entity[1] == 'x' || entity[1] == 'X');
      const size_t digitsStart = hexadecimal ? 2U : 1U;
      if (digitsStart == entity.size())
        throw runtime_error("Referência numérica XML vazia.");

      uint32_t codePoint = 0;
      const char *first = entity.data() + digitsStart;
      const char *last = entity.data() + entity.size();
      const auto result =
          from_chars(first, last, codePoint, hexadecimal ? 16 : 10);
      if (result.ec != errc{} || result.ptr != last)
        throw runtime_error("Referência numérica XML inválida.");
      appendUtf8(decoded, codePoint);
    } else {
      throw runtime_error("Entidade XML desconhecida: &" + entity + ";");
    }

    index = end + 1;
  }

  return decoded;
}

uintmax_t parseSize(const string &value) {
  uintmax_t parsed = 0;
  const char *first = value.data();
  const char *last = first + value.size();
  const auto result = from_chars(first, last, parsed);
  if (value.empty() || result.ec != errc{} || result.ptr != last)
    throw runtime_error("Tamanho XML inválido: " + value);
  return parsed;
}

void validateItemName(const string &name) {
  if (!Utils::nomeItemPortatilValido(name))
    throw runtime_error("Nome de item XML inválido.");
}

class XmlReader {
public:
  explicit XmlReader(string input) : source(move(input)) {
    if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
      position = 3;
  }

  unique_ptr<Diretoria> read() {
    skipWhitespace();
    if (startsWith("<?xml")) {
      const size_t declarationEnd = source.find("?>", position + 5);
      if (declarationEnd == string::npos)
        fail("Declaração XML incompleta.");
      position = declarationEnd + 2;
    }

    skipWhitespace();
    unique_ptr<Item> root = readItem(".");
    if (root->getIsFicheiro())
      fail("O elemento raiz tem de ser uma diretoria.");

    skipWhitespace();
    if (position != source.size())
      fail("Conteúdo adicional depois da diretoria raiz.");

    return unique_ptr<Diretoria>(static_cast<Diretoria *>(root.release()));
  }

private:
  struct StartTag {
    string name;
    unordered_map<string, string> attributes;
    bool selfClosing = false;
  };

  string source;
  size_t position = 0;

  [[noreturn]] void fail(const string &message) const {
    throw runtime_error(message + " (posição " + to_string(position) + ")");
  }

  bool startsWith(const string &token) const {
    return source.compare(position, token.size(), token) == 0;
  }

  bool skipWhitespace() {
    const size_t original = position;
    while (position < source.size() &&
           isspace(static_cast<unsigned char>(source[position])) != 0)
      ++position;
    return position != original;
  }

  void expect(char expected) {
    if (position >= source.size() || source[position] != expected)
      fail(string("Era esperado '") + expected + "'.");
    ++position;
  }

  string readIdentifier() {
    const size_t start = position;
    while (position < source.size()) {
      const unsigned char character =
          static_cast<unsigned char>(source[position]);
      if (isalnum(character) == 0 && character != '_' && character != '-')
        break;
      ++position;
    }
    if (start == position)
      fail("Identificador XML em falta.");
    return source.substr(start, position - start);
  }

  string readAttributeValue() {
    if (position >= source.size() ||
        (source[position] != '"' && source[position] != '\''))
      fail("Valor de atributo XML sem aspas.");

    const char quote = source[position++];
    const size_t start = position;
    while (position < source.size() && source[position] != quote) {
      if (source[position] == '<')
        fail("Carácter '<' não escapado num atributo XML.");
      ++position;
    }
    if (position == source.size())
      fail("Valor de atributo XML incompleto.");

    const string encoded = source.substr(start, position - start);
    ++position;
    return decodeXmlAttribute(encoded);
  }

  StartTag readStartTag() {
    expect('<');
    if (position < source.size() && source[position] == '/')
      fail("Tag de fecho inesperada.");

    StartTag tag;
    tag.name = readIdentifier();

    while (true) {
      const bool hadWhitespace = skipWhitespace();
      if (startsWith("/>")) {
        position += 2;
        tag.selfClosing = true;
        return tag;
      }
      if (position < source.size() && source[position] == '>') {
        ++position;
        return tag;
      }
      if (!hadWhitespace)
        fail("Falta espaço antes de um atributo XML.");

      const string attributeName = readIdentifier();
      skipWhitespace();
      expect('=');
      skipWhitespace();
      string value = readAttributeValue();
      if (!tag.attributes.emplace(attributeName, move(value)).second)
        fail("Atributo XML duplicado: " + attributeName);
    }
  }

  void readEndTag(const string &expectedName) {
    if (!startsWith("</"))
      fail("Tag de fecho em falta para " + expectedName + ".");
    position += 2;
    const string actualName = readIdentifier();
    skipWhitespace();
    expect('>');
    if (actualName != expectedName)
      fail("Tag de fecho inesperada: " + actualName);
  }

  static string requiredAttribute(const StartTag &tag, const string &name) {
    const auto found = tag.attributes.find(name);
    if (found == tag.attributes.end())
      throw runtime_error("Atributo obrigatório em falta: " + name);
    return found->second;
  }

  static void requireAttributeCount(const StartTag &tag, size_t expected) {
    if (tag.attributes.size() != expected)
      throw runtime_error("Conjunto de atributos inválido em <" + tag.name +
                          ">.");
  }

  unique_ptr<Item> readItem(const string &parentPath) {
    StartTag tag = readStartTag();
    if (tag.name == "diretoria")
      return readDirectory(move(tag), parentPath);
    if (tag.name == "ficheiro")
      return readFile(move(tag), parentPath);
    fail("Elemento XML desconhecido: " + tag.name);
  }

  unique_ptr<Item> readDirectory(StartTag tag, const string &parentPath) {
    requireAttributeCount(tag, 2);
    const string name = requiredAttribute(tag, "nome");
    const uintmax_t declaredSize =
        parseSize(requiredAttribute(tag, "tamanho"));
    validateItemName(name);
    if (tag.selfClosing)
      fail("Uma diretoria não pode usar uma tag autocontida.");

    const string path = (fs::path(parentPath) / name).generic_string();
    auto directory = make_unique<Diretoria>(name, path);
    while (true) {
      skipWhitespace();
      if (startsWith("</"))
        break;
      if (position >= source.size())
        fail("Diretoria sem tag de fecho: " + name);
      auto child = readItem(path);
      const bool duplicate = any_of(
          directory->getConteudo().begin(), directory->getConteudo().end(),
          [&child](const unique_ptr<Item> &existing) {
            return Utils::nomesItemEquivalentes(existing->getNome(),
                                                child->getNome());
          });
      if (duplicate)
        fail("Nome duplicado na diretoria " + name + ": " +
             child->getNome());
      directory->adicionar(move(child));
    }

    readEndTag("diretoria");
    const uintmax_t calculatedSize = directory->recalcularTamanho();
    if (calculatedSize != declaredSize)
      fail("Tamanho inconsistente na diretoria " + name + ".");
    return directory;
  }

  unique_ptr<Item> readFile(StartTag tag, const string &parentPath) {
    requireAttributeCount(tag, 4);
    const string name = requiredAttribute(tag, "nome");
    const uintmax_t size = parseSize(requiredAttribute(tag, "tamanho"));
    const string extension = requiredAttribute(tag, "extensao");
    const string modificationDate =
        requiredAttribute(tag, "dataModificacao");
    validateItemName(name);

    if (!tag.selfClosing) {
      skipWhitespace();
      readEndTag("ficheiro");
    }

    const string path = (fs::path(parentPath) / name).generic_string();
    return make_unique<Ficheiro>(name, path, size, extension,
                                 modificationDate);
  }
};

void ensureWritable(const ofstream &output) {
  if (!output)
    throw runtime_error("Falha ao escrever o documento XML.");
}

} // namespace

XML::~XML() {
  if (output.is_open())
    output.close();
}

void XML::WriteStartDocument(const string &ficheiro) {
  if (output.is_open())
    output.close();
  openDirectories = 0;
  output.open(ficheiro, ios::trunc);
  if (!output.is_open())
    throw runtime_error("Não foi possível criar o XML: " + ficheiro);
  output << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
  ensureWritable(output);
}

void XML::WriteEndDocument() {
  if (!output.is_open())
    return;
  if (openDirectories != 0)
    throw runtime_error("Existem diretorias XML por fechar.");
  output.flush();
  ensureWritable(output);
  output.close();
  ensureWritable(output);
}

void XML::WriteFile(const string &nome, uintmax_t tamanho,
                    const string &extensao, const string &dataModificacao) {
  const string tab = Utils::Tabulacao(openDirectories);
  output << tab << "<ficheiro nome=\"" << escapeXmlAttribute(nome)
         << "\" tamanho=\"" << tamanho << "\" extensao=\""
         << escapeXmlAttribute(extensao) << "\" dataModificacao=\""
         << escapeXmlAttribute(dataModificacao) << "\"></ficheiro>\n";
  ensureWritable(output);
}

void XML::WriteStartDirectory(const string &nome, uintmax_t tamanho) {
  const string tab = Utils::Tabulacao(openDirectories);
  output << tab << "<diretoria nome=\"" << escapeXmlAttribute(nome)
         << "\" tamanho=\"" << tamanho << "\">\n";
  ensureWritable(output);
  ++openDirectories;
}

void XML::WriteEndDirectory() {
  if (openDirectories == 0)
    throw runtime_error("Tentativa de fechar uma diretoria XML inexistente.");

  --openDirectories;
  const string tab = Utils::Tabulacao(openDirectories);
  output << tab << "</diretoria>\n";
  ensureWritable(output);
}

ifstream XML::ImportDocument(const string &ficheiro) {
  ifstream input(ficheiro, ios::binary);
  if (!input.is_open())
    Logger::log(Logger::Level::ERROR_, "Erro ao abrir o XML: " + ficheiro);
  return input;
}

unique_ptr<Diretoria> XML::ReadDocument(istream &ficheiro) {
  const string contents((istreambuf_iterator<char>(ficheiro)),
                        istreambuf_iterator<char>());
  if (!ficheiro.eof() && ficheiro.fail())
    throw runtime_error("Falha ao ler o documento XML.");
  return XmlReader(contents).read();
}
