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

size_t Diretoria::getNItens() const { return conteudo.size(); }
