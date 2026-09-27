#ifndef FICHEIRO_H
#define FICHEIRO_H

#include "IncludesGerais.h"
#include "Item.h"

class Ficheiro : public Item {
public:
  Ficheiro(const string &_nome, const string &_caminho);
  Ficheiro(const string &_nome, const string &_caminho, const int &_tamanho,
           string &_extensao, string &_dataModificacao);
  virtual ~Ficheiro();

  string getExtensao() const;
  string getDataModificacao() const;

protected:
private:
  string extensao;
  string dataModificacao;
};

#endif // FICHEIRO_H
