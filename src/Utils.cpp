#include "Utils.h"

/**
 * Resumo:
 * Utilitário que extrai o nome final (ficheiro ou diretoria folha) de um
 * caminho completo. Utiliza a biblioteca filesystem para fazer o parsing do
 * caminho. Se ocorrer alguma exceção durante o processo, retorna a string
 * original como salvaguarda.
 *
 * Parâmetros:
 * - path (const string&): O caminho completo (ex:
 * "C:/Docs/Trabalho/relatorio.pdf").
 *
 * Retorno:
 * - string: O nome extraído (ex: "relatorio.pdf") ou o caminho original em caso
 * de erro.
 */
string Utils::extrairNome(const string &path) {
  try {
    return fs::path(path).filename().string();
  } catch (const exception &) {
    return path; // devolve o original se algo correr mal
  }
}

/**
 * Resumo:
 * Configura a consola (especificamente em ambiente Windows) para utilizar
 * a codificação de caracteres UTF-8. Define tanto a página de código de
 * saída (OutputCP) como a de entrada (CP), permitindo a correta exibição
 * e leitura de caracteres especiais (acentos, cedilhas, símbolos).
 *
 * Parâmetros:
 * - Nenhum.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void Utils::UTF8() {
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8); // input
}

/**
 * Resumo:
 * Gera uma string contendo uma sequência de caracteres de tabulação ('\t').
 * É utilizada para criar indentação visual dinâmica na exibição hierárquica
 * (árvore) de ficheiros e diretorias, baseada no nível de profundidade.
 *
 * Parâmetros:
 * - n (int): O número de tabulações a gerar.
 *
 * Retorno:
 * - string: Uma string composta por 'n' caracteres de tabulação concatenados.
 */
string Utils::Tabulacao(size_t n) {
  string tab = "";
  for (size_t i = 0; i < n; i++)
    tab += "\t";
  return tab;
}

/**
 * Resumo:
 * Padroniza a formatação de um caminho de ficheiro para garantir consistência
 * nas comparações. Realiza três limpezas principais:
 * 1. Converte todas as barras invertidas ('\') do Windows para barras normais
 * ('/').
 * 2. Remove barras duplicadas acidentais (ex: "pasta//ficheiro" ->
 * "pasta/ficheiro").
 * 3. Remove a barra final se existir, para que diretorias não terminem em '/'.
 *
 * Parâmetros:
 * - path (const string&): O caminho original "bruto" que se pretende limpar.
 *
 * Retorno:
 * - string: Uma nova string contendo o caminho normalizado.
 */
string Utils::NormalizarCaminho(const string &path) {
  string normalizado = path;
  replace(normalizado.begin(), normalizado.end(), '\\', '/');

  // Remove duplas barras //
  while (normalizado.find("//") != string::npos)
    normalizado.erase(normalizado.find("//"), 1);

  // Remove / no fim
  if (!normalizado.empty() && normalizado.back() == '/')
    normalizado.pop_back();

  return normalizado;
}

/**
 * Resumo:
 * Função utilitária que percorre uma lista de strings e imprime cada elemento
 * na saída padrão (consola), inserindo uma quebra de linha após cada item.
 * Utilizada para listar resultados de pesquisas.
 *
 * Parâmetros:
 * - lista (list<string>&): Referência para a lista de strings que se pretende
 * exibir.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void Utils::PrintListaString(list<string> &lista) {
  for (const auto &s : lista)
    cout << s << endl;
}

/**
 * Resumo:
 * Verifica se uma determinada string ('palavra') está contida dentro de outra
 * string maior ('texto'). Utiliza o método 'find' da biblioteca padrão.
 * É frequentemente usada para verificar inclusão de caminhos (ex: saber se uma
 * pasta é subpasta de outra) ou para filtragem de nomes.
 *
 * Parâmetros:
 * - texto (const string&): A string principal onde será feita a busca.
 * - palavra (const string&): A sub-string que se pretende encontrar.
 *
 * Retorno:
 * - bool: Retorna 'true' se a 'palavra' existir em qualquer posição do 'texto',
 * ou 'false' caso contrário.
 */
bool Utils::contemPalavra(const string &texto, const string &palavra) {
  return texto.find(palavra) != string::npos;
}

/**
 * Resumo:
 * Gera uma string formatada contendo um número entre parênteses, com largura
 * fixa de 3 dígitos e preenchimento com zeros à esquerda (ex: o número 5
 * torna-se "(005)"). É utilizada para criar sufixos consistentes quando se
 * renomeiam ficheiros duplicados.
 *
 * Parâmetros:
 * - n (int): O número sequencial a ser formatado.
 *
 * Retorno:
 * - string: O sufixo formatado resultante.
 */
string Utils::gerarSufixo(int n) {
  stringstream ss;
  ss << "(" << setw(3) << setfill('0') << n << ")";
  return ss.str();
}

/**
 * Resumo:
 * Constrói um novo nome de ficheiro para resolver conflitos de nomes
 * duplicados. A função isola a parte do nome antes da última extensão (ponto),
 * insere o sufixo numérico gerado e volta a concatenar a extensão no final.
 * (Ex: transforma "relatorio.txt" e n=1 em "relatorio(001).txt").
 *
 * Parâmetros:
 * - nome (string): O nome original completo do ficheiro.
 * - n (int): O número sequencial da cópia a ser inserido no nome.
 * - extensao (string): A extensão do ficheiro (sem o ponto inicial) para ser
 * anexada ao fim.
 *
 * Retorno:
 * - string: O novo nome completo com o sufixo inserido na posição correta.
 */
string Utils::alterarNomeDuplicado(string nome, int n, string extensao) {
  fs::path caminho(nome);
  string base = caminho.stem().string();
  string sufixoExtensao = extensao.empty() ? "" : "." + extensao;
  return base + gerarSufixo(n) + sufixoExtensao;
}

void Utils::limparEcra() {
#ifdef _WIN32
  system("cls");
#else
  system("clear");
#endif
}

void Utils::esperarEnter() {
  cout << "Pressione Enter para continuar...";
  cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  cin.get();
}

/**
 * Resumo:
 * Identifica o nome da diretoria "pai" (imediata) onde um determinado item está
 * localizado, baseando-se no seu caminho completo. Utiliza a biblioteca
 * filesystem para navegar na estrutura do caminho. Caso o item não tenha pai
 * identificável (ex: está na raiz do sistema) ou o nome extraído seja vazio,
 * retorna o nome da diretoria raiz como salvaguarda.
 *
 * Parâmetros:
 * - caminho (const string&): O caminho completo do ficheiro ou diretoria em
 * questão.
 * - nomeRaiz (const string&): O nome da diretoria raiz do sistema, a ser
 * retornado caso o item esteja localizado no nível superior da hierarquia.
 *
 * Retorno:
 * - string: O nome da diretoria que contém o item (ou o nome da raiz se não
 * houver pai).
 */
string Utils::NomeDiretoriadoItem(const string &caminho,
                                  const string &nomeRaiz) {
  fs::path p = fs::path(caminho);

  // diretoria onde o ficheiro está
  fs::path parent = p.parent_path();

  if (parent.empty())
    return nomeRaiz;

  string nomeDiretoria = Utils::extrairNome(parent.string());

  if (nomeDiretoria.empty())
    return nomeRaiz;

  return nomeDiretoria;
}

/**
 * Resumo:
 * Extrai a parte inicial de um nome de ficheiro até encontrar o primeiro
 * parêntese de abertura '('. Esta função é utilizada essencialmente para
 * ignorar sufixos numéricos de cópias (ex: "Ficheiro(1)") e obter o nome base
 * original. Se o nome não contiver parênteses, retorna a string completa
 * inalterada.
 *
 * Parâmetros:
 * - nomeFicheiro (const string&): A string contendo o nome do ficheiro a
 * processar.
 *
 * Retorno:
 * - string: A substring desde o início até à posição do parêntese (exclusivo),
 * ou o nome original caso não exista separador.
 */
string Utils::nomeAteParentese(const string &nomeFicheiro) {
  size_t pos = nomeFicheiro.find('('); // encontra o primeiro '('
  if (pos == string::npos) {
    return nomeFicheiro; // não tem '(', devolve o nome inteiro
  }
  return nomeFicheiro.substr(0,
                             pos); // devolve da posição 0 até '(' (não inclui)
}
