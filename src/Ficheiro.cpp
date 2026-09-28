#include "Ficheiro.h"
#include "Logger.h"

//==================== Construtor e Destrutor ====================
/**
 * Resumo:
 * Construtor da classe Ficheiro. Inicializa o objeto invocando a classe base
 * 'Item' e tenta extrair automaticamente os metadados do sistema de ficheiros
 * físico. Calcula o tamanho em bytes, isola a extensão (removendo o ponto
 * inicial) e converte a data de última modificação para o formato "AAAA|MM|DD".
 * Em caso de exceção (ex: falha de acesso ao disco), define valores padrão
 * seguros e regista o erro.
 *
 * Parâmetros:
 * - _nome (const string&): O nome do ficheiro (ex: "relatorio.txt").
 * - _caminho (const string&): O caminho completo para o ficheiro no disco.
 */
Ficheiro::Ficheiro(const string &_nome, const string &_caminho)
    : Item(_nome, _caminho, true) {
  try {

    if (fs::exists(caminho) && fs::is_regular_file(caminho))
      tamanho = fs::file_size(caminho);

    extensao = fs::path(nome).extension().string();
    if (!extensao.empty() && extensao[0] == '.')
      extensao.erase(0, 1); // remove o ponto inicial (".txt" - "txt")

    auto ftime = fs::last_write_time(caminho);

    auto tmp = chrono::time_point_cast<chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + chrono::system_clock::now());
    time_t tempo = chrono::system_clock::to_time_t(tmp);

    stringstream ss;

    ss << put_time(localtime(&tempo), "%Y|%m|%d");
    dataModificacao = ss.str();

  } catch (const exception &e) {
    Logger::log(Logger::Level::ERROR_,
                "Erro ao obter informações do ficheiro '" + caminho +
                    "': " + e.what());
    extensao = ""; // se falhar por algum motivo
    tamanho = 0;
    dataModificacao = "Desconhecida";
  }
}

Ficheiro::Ficheiro(const string &_nome, const string &_caminho,
                   const int &_tamanho, string &_extensao,
                   string &_dataModificacao)
    : Item(_nome, _caminho, _tamanho, true) {
  extensao = _extensao;
  dataModificacao = _dataModificacao;
}

Ficheiro::~Ficheiro() {
  Logger::log(Logger::Level::INFO, "Ficheiro apagado: " + getNome());
}

//===========================================GET===========================================
string Ficheiro::getExtensao() const { return extensao; }

string Ficheiro::getDataModificacao() const { return dataModificacao; }
