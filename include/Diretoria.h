#ifndef DIRETORIA_H
#define DIRETORIA_H

#include "IncludesGerais.h"
#include "Item.h"
#include "Utils.h"

class Diretoria : public Item {
public:
  using Conteudo = list<unique_ptr<Item>>;

  Diretoria(const string &_nome, const string &_caminho);
  ~Diretoria() override = default;

  Diretoria(const Diretoria &) = delete;
  Diretoria &operator=(const Diretoria &) = delete;
  Diretoria(Diretoria &&) noexcept = default;
  Diretoria &operator=(Diretoria &&) noexcept = default;

  Conteudo &getConteudo();
  const Conteudo &getConteudoConst() const;
  void adicionar(unique_ptr<Item> item);
  size_t getNItens() const;

protected:
private:
  Conteudo conteudo;
};

#endif // DIRETORIA_H
