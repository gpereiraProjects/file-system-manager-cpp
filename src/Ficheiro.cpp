#include "Ficheiro.h"
#include "Logger.h"

using namespace std;
namespace fs = std::filesystem;

namespace {

bool localTime(const time_t &time, tm &result) {
#ifdef _WIN32
  return localtime_s(&result, &time) == 0;
#else
  return localtime_r(&time, &result) != nullptr;
#endif
}

} // namespace

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
    : Item(_nome, _caminho) {
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

    tm local{};
    if (!localTime(tempo, local))
      throw runtime_error("Não foi possível converter a data do ficheiro.");

    stringstream ss;
    ss << put_time(&local, "%Y|%m|%d");
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
                   uintmax_t _tamanho, const string &_extensao,
                   const string &_dataModificacao)
    : Item(_nome, _caminho, _tamanho) {
  extensao = _extensao;
  dataModificacao = _dataModificacao;
}

Ficheiro::~Ficheiro() {
  Logger::log(Logger::Level::INFO, "Ficheiro apagado: " + getNome());
}

//===========================================GET===========================================
const string &Ficheiro::getExtensao() const { return extensao; }

const string &Ficheiro::getDataModificacao() const { return dataModificacao; }

void Ficheiro::setNome(const string &novoNome) {
  Item::setNome(novoNome);
  extensao = fs::path(novoNome).extension().string();
  if (!extensao.empty() && extensao.front() == '.')
    extensao.erase(0, 1);
}
