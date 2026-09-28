#ifndef XML_H
#define XML_H

#include "IncludesGerais.h"
class Diretoria;

class XML {
private:
  std::size_t openDirectories = 0;
  std::ofstream output;

public:
  XML() = default;
  ~XML();

  XML(const XML &) = delete;
  XML &operator=(const XML &) = delete;

  //=============WRITE============
  void WriteStartDocument(const std::string &ficheiro);
  void WriteEndDocument();
  void WriteFile(const std::string &nome, std::uintmax_t tamanho,
                 const std::string &extensao,
                 const std::string &dataModificacao);
  void WriteStartDirectory(const std::string &nome, std::uintmax_t tamanho);
  void WriteEndDirectory();

  //=============READ=============
  std::ifstream ImportDocument(const std::string &ficheiro);
  std::unique_ptr<Diretoria> ReadDocument(std::istream &ficheiro);
};

#endif // XML_H
