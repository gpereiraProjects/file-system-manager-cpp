#include "Diretoria.h"
#include "Ficheiro.h"
#include "Logger.h"

//==================== Construtor e Destrutor ====================
Diretoria::Diretoria(const string &_nome, const string &_caminho)
    : Item(_nome, _caminho, false) {
  tamanho = 0;
}

Diretoria::~Diretoria() {
  for (Item *item : conteudo) {
    delete item; // apaga item (ficheiro ou diretoria)
  }
  conteudo.clear();
  Logger::log(Logger::Level::INFO, "Diretoria apagada: " + getNome());
}

//==================== Métodos Públicos ====================
list<Item *> &Diretoria::getConteudo() { return conteudo; }

const list<Item *> Diretoria::getConteudoConst() const { return conteudo; }

void Diretoria::adicionar(Item *i) {
  conteudo.push_back(i);
  tamanho += i->getTamanho();
}

size_t Diretoria::getNItens() const { return conteudo.size(); }
