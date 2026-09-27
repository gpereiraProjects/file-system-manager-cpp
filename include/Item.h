#ifndef ITEM_H
#define ITEM_H

#include "IncludesGerais.h"
#include "Utils.h"

class Item {
public:
  Item(const string &_nome, const string &_caminho, bool _isFicheiro);
  Item(const string &_nome, const string &_caminho, const int &_tamanho,
       bool _isFicheiro);
  virtual ~Item();

  //============GET============
  string getNome();
  string getCaminho();
  uintmax_t getTamanho();
  bool getIsFicheiro();

  //============SET============
  void setCaminho(const string &novoCaminho);
  void setNome(const string &novoNome);

protected:
  string nome;
  string caminho;
  uintmax_t tamanho;
  bool isFicheiro; // verdadeiro se for ficheiro, falso se for diretoria

private:
};

#endif // ITEM_H
