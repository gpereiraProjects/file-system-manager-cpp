#include "Logger.h"
#include "IncludesGerais.h"

using namespace std;

namespace {

bool localTime(const time_t &time, tm &result) {
#ifdef _WIN32
    return localtime_s(&result, &time) == 0;
#else
    return localtime_r(&time, &result) != nullptr;
#endif
}

} // namespace

// Retorna a instância única do Logger (padrão Singleton)
Logger& Logger::instance() {
    // Cria a instância estática apenas na primeira vez que a função é chamada
    // Todas as chamadas futuras vão retornar a mesma instância
    static Logger logger;
    return logger;  // devolve a referência à instância única
}

// Inicializa o ficheiro de log com o caminho fornecido
void Logger::init(const string& path) {
    // Abre o ficheiro em modo append (acrescenta no final)
    instance().file.open(path, ios::app);

    // Se não foi possível abrir o ficheiro, lança uma exceção
    if (!instance().file)
        throw runtime_error("Não foi possível abrir o ficheiro de log.");
}

// Converte o nível de log (enum Level) para uma string legível
string Logger::levelToString(Level lvl) {
    switch (lvl) {
        case Level::INFO:    return "INFO";     // nível informativo
        case Level::WARNING: return "WARNING";  // nível aviso
        case Level::ERROR_:  return "ERROR";    // nível erro
        case Level::DEBUG:   return "DEBUG";    // nível debug
    }
    // Caso o valor não corresponda a nenhum nível (fallback)
    return "UNKNOWN";
}

// Cria um timestamp atual no formato "YYYY-MM-DD HH:MM:SS"
string Logger::timestamp() {
    time_t t = time(nullptr);                        // obtém o tempo atual
    tm local{};
    if (!localTime(t, local))
        return "timestamp-indisponivel";
    char buffer[32];                                 // buffer para armazenar a string
    if (strftime(buffer, sizeof(buffer),             // formata o tempo
                 "%Y-%m-%d %H:%M:%S", &local) == 0)
        return "timestamp-indisponivel";
    return buffer;                                   // devolve o timestamp como string
}

// Escreve uma mensagem no ficheiro de log
void Logger::log(Level level, const string& message) {
    Logger& inst = instance();                       // obtém a instância única do Logger

    lock_guard<mutex> lock(inst.mtx);               // bloqueia o mutex enquanto escreve (thread-safe)

    // Escreve no ficheiro:
    // [timestamp] [nível] mensagem
    inst.file << "[" << inst.timestamp() << "] "
              << "[" << inst.levelToString(level) << "] "
              << message << endl;                   // endl também faz flush para garantir escrita imediata
}
