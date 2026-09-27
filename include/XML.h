#ifndef XML_H
#define XML_H

#include "IncludesGerais.h"
#include "Utils.h"

class Diretoria;
class Ficheiro;

using namespace std;

class XML {
private:
  list<string> PTAG;
  ofstream FicheiroExp;
  ifstream FicheiroImp;

public:
  XML();
  virtual ~XML();

  //=============WRITE============
  void WriteStartDocument(string ficheiro);
  void WriteEndDocument();
  void WriteFile(string nome, uintmax_t tamanho, string extensao,
                 string dataModificacao);
  void WriteStartDirectory(string nome, uintmax_t tamanho);
  void WriteEndDirectory();

  //=============READ=============
  ifstream ImportDocument(const string &ficheiro);
  void ReadDirectory(ifstream &ficheiro, Diretoria *dirAtual);
};

#endif // XML_H
