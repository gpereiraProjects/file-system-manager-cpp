#ifndef LOGGER_H
#define LOGGER_H

#include "IncludesGerais.h"

/**
 * Classe Logger estática (implementada como instância única).
 * Permite escrever mensagens de log a partir de qualquer parte do código,
 * sem necessidade de criar objetos ou passar referências.
 */
class Logger {
public:
  // Níveis de log possíveis
  enum class Level { INFO, WARNING, ERROR_, DEBUG };

  /**
   * Inicializa o Logger com o caminho do ficheiro de log.
   * Deve ser chamado 1 vez no início do programa.
   */
  static void init(const std::string &path);

  /**
   * Escreve uma entrada no ficheiro de log.
   * Pode ser chamado de qualquer lugar do programa.
   */
  static void log(Level level, const std::string &message);

private:
  std::ofstream file; // Ficheiro onde o log é escrito
  std::mutex mtx;     // Mutex usado para tornar a escrita thread-safe

  Logger() = default; // Construtor privado (garantir que existe apenas uma
                      // única instância de uma classe e que essa instância é
                      // acessível globalmente)

  // Método interno que devolve a instância única do Logger
  static Logger &instance();

  // Gera timestamp atual como string
  std::string timestamp();

  // Converte enum Level para string
  std::string levelToString(Level lvl);
};

#endif
