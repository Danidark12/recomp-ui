/* recomp_cheats.h - lista de cheats (formato GameShark) compartilhada entre o
 * launcher e o runtime do jogo.
 *
 * Arquivo: cheats.txt ao lado do executável. Formato:
 *
 *   # comentário (preservado no topo do arquivo)
 *   [x] Nome em português | Name in English | Nombre en español
 *   > Descrição em português | Description in English | Descripción en español
 *
 * O terceiro campo (espanhol) é opcional.
 *   801F5F80 FFFF
 *   [ ] Outro cheat
 *   301E5C41 0000
 *
 * "[x]" = ligado, "[ ]" = desligado. O launcher e o menu dentro do jogo
 * gravam de volta só essas marcas (o resto do arquivo é reescrito igual).
 */
#ifndef RECOMP_CHEATS_H
#define RECOMP_CHEATS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RECOMP_CHEATS_MAX       96
#define RECOMP_CHEATS_MAX_CODES 48
#define RECOMP_CHEATS_TEXT      128
#define RECOMP_CHEATS_NOTE      320

typedef struct RecompCheat {
    char     name_pt[RECOMP_CHEATS_TEXT];
    char     name_en[RECOMP_CHEATS_TEXT];
    char     note_pt[RECOMP_CHEATS_NOTE];
    char     note_en[RECOMP_CHEATS_NOTE];
    char     name_es[RECOMP_CHEATS_TEXT];
    char     note_es[RECOMP_CHEATS_NOTE];
    int      enabled;
    int      code_count;
    uint32_t addr[RECOMP_CHEATS_MAX_CODES];   /* palavra de 8 dígitos */
    uint16_t value[RECOMP_CHEATS_MAX_CODES];  /* palavra de 4 dígitos */
    int      supported;                       /* todos os tipos de código conhecidos */
} RecompCheat;

typedef struct RecompCheatList {
    int         count;
    RecompCheat cheats[RECOMP_CHEATS_MAX];
    char        header[2048];                 /* comentários do topo do arquivo */
    int         loaded;                       /* 1 se cheats.txt foi lido */
} RecompCheatList;

/* Caminho de cheats.txt (pasta do executável). */
const char* recomp_cheats_path(void);

/* Lê cheats.txt. Retorna 1 se o arquivo existe. */
int recomp_cheats_load(RecompCheatList* list);

/* Regrava cheats.txt com o estado atual (ligado/desligado). */
int recomp_cheats_save(const RecompCheatList* list);

/* Idiomas: 0 = inglês, 1 = português, 2 = espanhol. */
#define RECOMP_CHEAT_LANG_EN 0
#define RECOMP_CHEAT_LANG_PT 1
#define RECOMP_CHEAT_LANG_ES 2

/* Idioma do launcher (launcher_i18n_language) convertido para o código acima. */
int recomp_cheat_lang_from_code(const char* code);

/* Nome/descrição no idioma pedido. Cai para o inglês (ou o português) quando
 * o pedido está vazio. */
const char* recomp_cheat_name(const RecompCheat* c, int lang);
const char* recomp_cheat_note(const RecompCheat* c, int lang);

/* 1 se o tipo de código (byte alto da palavra de endereço) é suportado. */
int recomp_cheat_code_type_supported(uint32_t addr_word);

#ifdef __cplusplus
}
#endif

#endif /* RECOMP_CHEATS_H */
