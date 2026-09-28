#ifndef XML_H
#define XML_H

#include "IncludesGerais.h"
#include "Utils.h"

class Diretoria;
class Ficheiro;

class XML {
private:
  std::list<std::string> PTAG;
  std::ofstream FicheiroExp;
  std::ifstream FicheiroImp;

public:
  XML();
  virtual ~XML();

  //=============WRITE============
  void WriteStartDocument(std::string ficheiro);
  void WriteEndDocument();
  void WriteFile(std::string nome, std::uintmax_t tamanho,
                 std::string extensao, std::string dataModificacao);
  void WriteStartDirectory(std::string nome, std::uintmax_t tamanho);
  void WriteEndDirectory();

  //=============READ=============
  std::ifstream ImportDocument(const std::string &ficheiro);
  bool ReadDirectory(std::ifstream &ficheiro, Diretoria *dirAtual);
};

#endif // XML_H
