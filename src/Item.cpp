#include "Item.h"

//==================== Construtor e Destrutor ====================
Item::Item(const string &_nome, const string &_caminho, bool _isFicheiro) {
  nome = _nome;
  caminho = _caminho;
  tamanho = 0;
  isFicheiro = _isFicheiro;
}

Item::Item(const string &_nome, const string &_caminho, const int &_tamanho,
           bool _isFicheiro) {
  nome = _nome;
  caminho = _caminho;
  tamanho = _tamanho;
  isFicheiro = _isFicheiro;
}
Item::~Item() {
  // dtor
}

//===========================================GET===========================================
string Item::getNome() { return nome; }

string Item::getCaminho() { return caminho; }

uintmax_t Item::getTamanho() { return tamanho; }

bool Item::getIsFicheiro() { return isFicheiro; }

//===========================================SET===========================================
void Item::setCaminho(const string &novoCaminho) { caminho = novoCaminho; }

void Item::setNome(const string &novoNome) { nome = novoNome; }
