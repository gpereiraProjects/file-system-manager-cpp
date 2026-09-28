#ifndef DIRETORIA_H
#define DIRETORIA_H

#include "IncludesGerais.h"
#include "Item.h"
#include "Utils.h"

class Diretoria : public Item {
public:
  using Conteudo = std::list<std::unique_ptr<Item>>;

  Diretoria(const std::string &nome, const std::string &caminho);
  ~Diretoria() override = default;

  Diretoria(const Diretoria &) = delete;
  Diretoria &operator=(const Diretoria &) = delete;
  Diretoria(Diretoria &&) noexcept = default;
  Diretoria &operator=(Diretoria &&) noexcept = default;

  const Conteudo &getConteudo() const;
  void adicionar(std::unique_ptr<Item> item);
  std::unique_ptr<Item> extrair(Item *item);
  std::uintmax_t recalcularTamanho();
  std::size_t getNItens() const;
  bool getIsFicheiro() const override { return false; }

private:
  Conteudo conteudo;
};

#endif // DIRETORIA_H
