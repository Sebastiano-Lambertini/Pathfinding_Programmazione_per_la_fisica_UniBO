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


// ==========================
// FUNZIONI DI UTILITÀ
// ==========================

size_t salta_virgolette_spazi(const std::string &linea, size_t inizio);

int trova_simbolo(const std::string &linea);

Associazione_id_simbolo ids_a_simboli(const std::string &linea);

std::vector<int> trova_associazione_per_id_vietati_o_pieni(
    const std::vector<Associazione_id_simbolo> &ids_simboli,
    const std::vector<int> &vietati_o_pieni);

void calcola_fattore_scala(Griglia &griglia);

std::vector<Punto> estrai_coordinate(const std::string &linea,
                                     const Griglia &griglia);

void calcola_dimensioni_max_min(
    const std::vector<oggetto_poligonale_vietato> &oggetti, Griglia &griglia);

void mondo_a_griglia(double x, double y, int &gx, int &gy,
                     Griglia &griglia);

bool calcola_intersezione(int x1, int y1, int x2, int y2, int y, int &x);

void riempi_poligoni(Griglia &griglia,
                     const std::vector<std::pair<int, int>> &poligono);

void chiedi_verde3_oltrepassabile(std::vector<int> &ids_vietati,
                                  std::vector<int> &ids_pieni);

void oggetti_a_griglia(const oggetto_poligonale_vietato &oggetto,
                       Griglia &griglia);

void dilata_ostacoli(Griglia &griglia);

double costo_percorso_geometrico_mondo(const std::vector<Punto> &percorso,
                                        const Griglia &griglia);

void esporta_percorso_omap(
    const std::string &nome_file_input, const std::string &nome_file_output,
    const std::vector<Percorso> &percorsi,
    const std::vector<Associazione_id_simbolo> &ids_simboli,  const Griglia &griglia);

// ==========================
// PIPELINE MODULARE
// ==========================

std::vector<Associazione_id_simbolo> leggi_ids_simboli(std::ifstream &file);

Insiemi_simboli costruisci_insiemi_simboli(
    const std::vector<Associazione_id_simbolo> &ids_simboli);

Posizioni_partenza_arrivo trova_partenza_arrivo(const std::vector<Associazione_id_simbolo> &ids_simboli);

Dati_mappa carica_oggetti_mappa(std::ifstream &file, int id_partenza,
                                int id_arrivo,
                                const std::unordered_set<int> &id_vietati,
                                const std::unordered_set<int> &id_pieni,
                                const Griglia &griglia);

void costruisci_griglia(const std::vector<oggetto_poligonale_vietato> &oggetti,
                   const Griglia &griglia);

void
converti_partenza_arrivo_griglia(const Punto &partenza_mondo,
                                 const Punto &arrivo_mondo, Griglia &griglia);

void stampa_statistiche_percorsi(const std::vector<Percorso> &percorsi,
                                 const Griglia &griglia);

void esporta_griglia_csv(const std::string &nome_file, const Griglia &griglia);

// ==========================
// API PUBBLICA
// ==========================

std::vector<Percorso> aiuto_main(const std::string &nome_file);

std::vector<Percorso> aiuto_main2(const std::string &nome_file);

} // namespace pf

#endif