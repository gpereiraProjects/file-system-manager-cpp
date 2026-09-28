#include "XML.h"
#include "Diretoria.h"
#include "Ficheiro.h"
#include "Logger.h"

using namespace std;

//==================== Construtor e Destrutor ====================
XML::XML() {
  // ctor
}

XML::~XML() { WriteEndDocument(); }

//===========================================WRITE===========================================
/**
 * Resumo:
 * Inicializa o processo de exportação XML. Abre o ficheiro de saída no disco
 * com o nome/caminho especificado e escreve o cabeçalho padrão de declaração
 * XML
 * (<?xml ... ?>), preparando o ficheiro para receber a estrutura de dados.
 *
 * Parâmetros:
 * - ficheiro (string): O caminho ou nome do ficheiro onde o conteúdo XML será
 * gravado.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void XML::WriteStartDocument(string ficheiro) {
  FicheiroExp.open(ficheiro);
  FicheiroExp << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
}

void XML::WriteEndDocument() { FicheiroExp.close(); }

/**
 * Resumo:
 * Escreve um elemento XML completo (`<ficheiro>`) no ficheiro de exportação.
 * A função calcula a indentação correta com base na profundidade atual (gerida
 * externamente pela estrutura 'PTAG') e insere os metadados do ficheiro (nome,
 * tamanho, extensão e data) como atributos da tag XML.
 *
 * Parâmetros:
 * - nome (string): O nome do ficheiro a ser registado.
 * - tamanho (uintmax_t): O tamanho do ficheiro em bytes.
 * - extensao (string): A extensão do ficheiro (sem o ponto).
 * - dataModificacao (string): A data formatada da última alteração.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void XML::WriteFile(string nome, uintmax_t tamanho, string extensao,
                    string dataModificacao) {
  string tab = Utils::Tabulacao(PTAG.size());
  FicheiroExp << tab << "<ficheiro nome=\"" << nome << "\" tamanho=\""
              << tamanho << "\" extensao=\"" << extensao
              << "\" dataModificacao=\"" << dataModificacao << "\"></ficheiro>"
              << endl;
}

/**
 * Resumo:
 * Escreve a tag de abertura de uma diretoria (`<diretoria ...>`) no ficheiro
 * XML. Calcula a indentação necessária baseada na profundidade atual e insere
 * os atributos de nome e tamanho. Adicionalmente, empilha um marcador na
 * estrutura de controlo 'PTAG', sinalizando que o sistema entrou num novo nível
 * de profundidade (para gerir futuras indentações e fechos de tags).
 *
 * Parâmetros:
 * - nome (string): O nome da diretoria a ser registada.
 * - tamanho (uintmax_t): O tamanho total ocupado pela diretoria (em bytes).
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void XML::WriteStartDirectory(string nome, uintmax_t tamanho) {
  string tab = Utils::Tabulacao(PTAG.size());
  FicheiroExp << tab << "<diretoria nome=\"" << nome << "\" tamanho=\""
              << tamanho << "\">" << endl;
  PTAG.push_front("diretoria");
}

/**
 * Resumo:
 * Escreve a tag de encerramento (ex: `</diretoria>`) no ficheiro XML,
 * finalizando o nível hierárquico atual. A função consulta e remove o último
 * elemento da pilha de controlo 'PTAG' para garantir o emparelhamento correto
 * das tags e ajusta a indentação. Inclui validação para evitar erros de
 * estrutura (tentar fechar sem abrir), registando um aviso no Logger se a pilha
 * estiver vazia.
 *
 * Parâmetros:
 * - Nenhum (utiliza o estado interno da pilha 'PTAG').
 *
 * Retorno:
 * - void (Não retorna valor).
 */
void XML::WriteEndDirectory() {
  if (PTAG.size() == 0)
    Logger::log(Logger::Level::WARNING,
                "Tentativa de fechar diretoria sem diretoria aberta.");
  else {
    string tab = Utils::Tabulacao(PTAG.size() - 1);
    string el = PTAG.front();
    FicheiroExp << tab << "</" << el << ">" << endl;
    PTAG.pop_front();
  }
}

//===========================================IMPORT===========================================
/**
 * Resumo:
 * Tenta abrir um ficheiro XML existente para leitura/importação. O fluxo é
 * devolvido por valor e gere automaticamente os seus próprios recursos. Se a
 * abertura falhar, regista o erro no Logger e devolve um fluxo fechado.
 *
 * Parâmetros:
 * - ficheiro (const string&): O caminho ou nome do ficheiro XML a ser lido.
 *
 * Retorno:
 * - ifstream: Fluxo aberto e pronto a ler, ou um fluxo fechado em caso de erro.
 */
ifstream XML::ImportDocument(const string &ficheiro) {
  ifstream input(ficheiro);
  if (!input.is_open()) {
    Logger::log(Logger::Level::ERROR_, "Erro ao abrir o XML: " + ficheiro);
  }
  return input;
}

/**
 * Resumo:
 * Reconstrói a hierarquia do sistema de ficheiros lendo sequencialmente o fluxo
 * XML. Utiliza expressões regulares (Regex) para interpretar as linhas e
 * identificar:
 * 1. Abertura de diretoria: Cria um novo objeto Diretoria, invoca-se
 * recursivamente para ler o seu conteúdo e adiciona-a à diretoria atual.
 * 2. Ficheiro: Extrai os atributos (nome, tamanho, extensão, data), cria o
 * objeto Ficheiro e adiciona-o à diretoria atual.
 * 3. Fecho de diretoria: Interrompe o ciclo de leitura atual (break),
 * retornando o controlo ao nível anterior da recursão.
 *
 * Parâmetros:
 * - ficheiro (ifstream&): Referência para o fluxo do ficheiro XML aberto para
 * leitura.
 * - dirAtual (Diretoria*): Ponteiro para a diretoria onde os itens lidos serão
 * inseridos.
 *
 * Retorno:
 * - void (Não retorna valor).
 */
bool XML::ReadDirectory(ifstream &ficheiro, Diretoria *dirAtual) {
  string linha;

  // static const evita recriar a regex em cada recursão
  static const regex regexDiretoriaAberta(
      "<diretoria\\s+nome=\"([^\"]+)\"\\s+tamanho=\"([^\"]+)\">");

  // Atenção: ajustei a regexFicheiro para ser mais legível
  static const regex regexFicheiro(
      "<ficheiro\\s+nome=\"([^\"]+)\"\\s+tamanho=\"([^\"]+)\"\\s+extensao=\"([^"
      "\"]*)\"\\s+dataModificacao=\"([^\"]+)\"></ficheiro>");

  static const regex regexDiretoriaFecho("</diretoria>");

  smatch match;

  while (getline(ficheiro, linha)) {
    // Remove CR (Windows) de forma segura
    if (!linha.empty() && linha.back() == '\r')
      linha.pop_back();

    // 1. Diretoria Aberta -> Recursão
    if (regex_search(linha, match, regexDiretoriaAberta)) {
      string nome = match[1];
      // int tamanho = stoi(match[2]);

      auto nova =
          make_unique<Diretoria>(nome, dirAtual->getCaminho() + "/" + nome);

      // MERGULHA (Recursão)
      if (!ReadDirectory(ficheiro, nova.get()))
        return false;

      // Depois de voltar da recursão (quando encontrou </diretoria>), adiciona
      // à atual
      dirAtual->adicionar(move(nova));
    }
    // 2. Ficheiro -> Adiciona e continua no loop
    else if (regex_search(linha, match, regexFicheiro)) {
      string nome = match[1];
      int tamanho = stoi(match[2]);
      string extensao = match[3];
      string dataModificacao = match[4];

      // Se o construtor aceitar mais dados, passa-os aqui
      auto novo = make_unique<Ficheiro>(
          nome, dirAtual->getCaminho() + "/" + nome, tamanho, extensao,
          dataModificacao);
      dirAtual->adicionar(move(novo));
    }
    // 3. Fecho de Diretoria -> Sai da recursão atual
    else if (regex_search(linha, regexDiretoriaFecho)) {
      return true;
    }
  }
  return false;
}
