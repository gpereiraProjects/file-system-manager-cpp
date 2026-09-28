#include "Diretoria.h"
#include "Ficheiro.h"

//==================== Construtor e Destrutor ====================
Diretoria::Diretoria(const string &_nome, const string &_caminho)
    : Item(_nome, _caminho, false) {
  tamanho = 0;
}

//==================== Métodos Públicos ====================
Diretoria::Conteudo &Diretoria::getConteudo() { return conteudo; }

const Diretoria::Conteudo &Diretoria::getConteudoConst() const {
  return conteudo;
}

void Diretoria::adicionar(unique_ptr<Item> item) {
  tamanho += item->getTamanho();
  conteudo.push_back(move(item));
}

unique_ptr<Item> Diretoria::extrair(Item *item) {
  for (auto it = conteudo.begin(); it != conteudo.end(); ++it) {
    if (it->get() == item) {
      auto extraido = move(*it);
      conteudo.erase(it);
      recalcularTamanho();
      return extraido;
    }
  }
  return nullptr;
}

uintmax_t Diretoria::recalcularTamanho() {
  tamanho = 0;
  for (const auto &item : conteudo) {
    if (auto *subdiretoria = dynamic_cast<Diretoria *>(item.get()))
      subdiretoria->recalcularTamanho();
    tamanho += item->getTamanho();
  }
  return tamanho;
}

size_t Diretoria::getNItens() const { return conteudo.size(); }
