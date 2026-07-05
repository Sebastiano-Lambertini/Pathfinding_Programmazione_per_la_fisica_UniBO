#ifndef PARSER_H
#define PARSER_H

#include "path.hpp"

#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace pf {

// ==========================
// STRUTTURE BASE
// ==========================

struct Associazione_id_simbolo {
  int id;
  int simbolo;

  Associazione_id_simbolo() : id(0), simbolo(0) {}

  Associazione_id_simbolo(int valore_id, int valore_simbolo)
      : id(valore_id), simbolo(valore_simbolo) {}
};

struct oggetto_poligonale_vietato {
  int id;
  std::vector<Punto> contorno;
  bool e_pieno;
};

struct Dati_costo_percorsi {
  double min_x;
  double max_x;
  double min_y;
  double max_y;

  int larghezza_griglia;
  int altezza_griglia;

  double fattore_scala;
};

// ==========================
// STRUTTURE SUPPORTO PIPELINE
// ==========================

struct Insiemi_simboli {
  std::unordered_set<int> vietati;
  std::unordered_set<int> pieni;
};

struct Posizioni_partenza_arrivo {
  int partenza = -1;
  int arrivo = -1;
};

struct Dati_mappa {
  Punto partenza_mondo;
  Punto arrivo_mondo;

  bool partenza_trovata = false;
  bool arrivo_trovato = false;

  std::vector<oggetto_poligonale_vietato> oggetti_vietati;
};

struct Dati_griglia {
  std::vector<std::vector<TipoCella>> griglia;

  double min_x = 0;
  double max_x = 0;
  double min_y = 0;
  double max_y = 0;

  int larghezza = 0;
  int altezza = 0;
};

struct Partenza_arrivo_griglia {
  Punto partenza;
  Punto arrivo;
};

// ==========================
// FUNZIONI DI UTILITÀ
// ==========================

size_t salta_virgolette_spazi(
    const std::string& linea,
    size_t inizio);

int trova_simbolo(
    const std::string& linea);

Associazione_id_simbolo ids_a_simboli(
    const std::string& linea);

std::vector<int>
trova_associazione_per_id_vietati_o_pieni(
    const std::vector<Associazione_id_simbolo>& ids_simboli,
    const std::vector<int>& vietati_o_pieni);

double calcola_fattore_scala();

std::vector<Punto> estrai_coordinate(
    const std::string& linea,
    double fattore_scala);

void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato>& oggetti,
    double& min_x,
    double& max_x,
    double& min_y,
    double& max_y,
    const Punto& arrivo,
    const Punto& partenza);

void mondo_a_griglia(
    double x,
    double y,
    double min_x,
    double max_x,
    double min_y,
    double max_y,
    int larghezza_griglia,
    int altezza_griglia,
    int& gx,
    int& gy);

bool calcola_intersezione(
    int x1,
    int y1,
    int x2,
    int y2,
    int y,
    int& x);

void riempi_poligoni(
    std::vector<std::vector<TipoCella>>& griglia,
    const std::vector<std::pair<int, int>>& poligono,
    int larghezza_griglia,
    int altezza_griglia);

void chiedi_verde3_oltrepassabile(
    std::vector<int>& ids_vietati,
    std::vector<int>& ids_pieni);

void oggetti_a_griglia(
    const oggetto_poligonale_vietato& oggetto,
    std::vector<std::vector<TipoCella>>& griglia,
    double min_x,
    double max_x,
    double min_y,
    double max_y,
    int larghezza_griglia,
    int altezza_griglia);

void dilata_ostacoli(
    std::vector<std::vector<TipoCella>>& griglia);

double costo_percorso_geometrico_mondo(
    const std::vector<Punto>& percorso,
    double min_x,
    double max_x,
    double min_y,
    double max_y,
    int larghezza_griglia,
    int altezza_griglia,
    double fattore_scala);

void esporta_percorso_omap(
    const std::string& nome_file_input,
    const std::string& nome_file_output,
    const std::vector<Percorso>& percorsi,
    const std::vector<Associazione_id_simbolo>& ids_simboli,
    double min_x,
    double max_x,
    double min_y,
    double max_y,
    int larghezza_griglia,
    int altezza_griglia,
    double fattore_scala);

// ==========================
// PIPELINE MODULARE
// ==========================

std::vector<Associazione_id_simbolo>
leggi_ids_simboli(
    std::ifstream& file);

Insiemi_simboli costruisci_insiemi_simboli(
    const std::vector<Associazione_id_simbolo>& ids_simboli);

Posizioni_partenza_arrivo trova_partenza_arrivo(
    const std::vector<Associazione_id_simbolo>& ids_simboli);

Dati_mappa carica_oggetti_mappa(
    std::ifstream& file,
    int id_partenza,
    int id_arrivo,
    const std::unordered_set<int>& id_vietati,
    const std::unordered_set<int>& id_pieni,
    double fattore_scala);

Dati_griglia costruisci_griglia(
    const std::vector<oggetto_poligonale_vietato>& oggetti,
    const Punto& partenza,
    const Punto& arrivo);

Partenza_arrivo_griglia converti_partenza_arrivo_griglia(
    const Dati_griglia& dati,
    const Punto& partenza_mondo,
    const Punto& arrivo_mondo);

void stampa_statistiche_percorsi(
    const std::vector<Percorso>& percorsi,
    const Dati_costo_percorsi& dati);

void esporta_griglia_csv(
    const std::string& nome_file,
    const std::vector<std::vector<TipoCella>>& griglia,
    const Punto& partenza,
    const Punto& arrivo,
    bool partenza_valida,
    bool arrivo_valido);

// ==========================
// API PUBBLICA
// ==========================

std::vector<Percorso> aiuto_main(
    const std::string& nome_file);

std::vector<Percorso> aiuto_main2(
    const std::string& nome_file);

} // namespace pf

#endif