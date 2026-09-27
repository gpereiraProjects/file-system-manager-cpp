#ifndef UTILS_H
#define UTILS_H

#include "IncludesGerais.h"

class Utils {
private:
  Utils() = delete;  // impede criar objetos
  ~Utils() = delete; // impede destruir objetos

public:
  static string extrairNome(const string &path);
  static void UTF8();
  static string Tabulacao(int n);
  static string NormalizarCaminho(const string &path);
  static void PrintListaString(list<string> &lista);
  static bool contemPalavra(const string &texto, const string &palavra);
  static string gerarSufixo(int n);
  static string alterarNomeDuplicado(string nome, int n, string extensao);
  static string NomeDiretoriadoItem(const string &caminho,
                                    const string &nomeRaiz);
  static void limparEcra();
  static void esperarEnter();
  static string nomeAteParentese(const string &nomeFicheiro);
};

#endif // UTILS_H
