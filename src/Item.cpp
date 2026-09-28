#include "Item.h"

using namespace std;

//==================== Construtor e Destrutor ====================
Item::Item(const string &nomeInicial, const string &caminhoInicial)
    : nome(nomeInicial), caminho(caminhoInicial), tamanho(0) {}

Item::Item(const string &nomeInicial, const string &caminhoInicial,
           uintmax_t tamanhoInicial)
    : nome(nomeInicial), caminho(caminhoInicial), tamanho(tamanhoInicial) {}

//===========================================GET===========================================
const string &Item::getNome() const { return nome; }

const string &Item::getCaminho() const { return caminho; }

uintmax_t Item::getTamanho() const { return tamanho; }

//===========================================SET===========================================
void Item::setCaminho(const string &novoCaminho) { caminho = novoCaminho; }

void Item::setNome(const string &novoNome) { nome = novoNome; }
