#pragma once

const char* launcher_i18n_language(void);
const char* launcher_i18n_text(const char* english);
/* Troca o idioma do launcher na hora ("en" ou "pt-BR"); persist != 0 grava a
 * escolha em launcher_language.txt ao lado do executável. */
void launcher_i18n_set_language(const char* lang, int persist);
