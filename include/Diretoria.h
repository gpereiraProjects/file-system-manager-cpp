#ifndef DIRETORIA_H
#define DIRETORIA_H

#include "IncludesGerais.h"
#include "Item.h"
#include "Utils.h"

class Diretoria : public Item {
public:
  Diretoria(const string &_nome, const string &_caminho);
  virtual ~Diretoria();

  list<Item *> &getConteudo();
  const list<Item *> getConteudoConst() const;
  void adicionar(Item *i);
  size_t getNItens() const;

protected:
private:
  list<Item *> conteudo;
};

#endif // DIRETORIA_H
