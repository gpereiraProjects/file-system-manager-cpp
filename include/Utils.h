#ifndef UTILS_H
#define UTILS_H

#include "IncludesGerais.h"

class Utils {
private:
  Utils() = delete;  // impede criar objetos
  ~Utils() = delete; // impede destruir objetos

public:
  static std::string extrairNome(const std::string &path);
  static void UTF8();
  static std::string Tabulacao(std::size_t n);
  static std::string NormalizarCaminho(const std::string &path);
  static bool nomeItemPortatilValido(const std::string &nome);
  static bool nomesItemEquivalentes(const std::string &left,
                                    const std::string &right);
  static void PrintListaString(std::list<std::string> &lista);
  static bool contemPalavra(const std::string &texto,
                            const std::string &palavra);
  static std::string gerarSufixo(int n);
  static std::string alterarNomeDuplicado(std::string nome, int n,
                                          std::string extensao);
  static void limparEcra();
  static void esperarEnter();
};

#endif // UTILS_H
