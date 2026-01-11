#import "@preview/diatypst:0.8.0": *
#let mpi_weak = csv("csv/strong_mpi.csv")
#let mpi_strong = csv("csv/weak_mpi.csv")

#show: slides.with(
  title: "Binarizzazione di una matrice", 
  subtitle: "Progetto - Sistemi Concorrenti e Paralleli M",
  date: "A.A. 2025-2026",
  authors: ("Davide Chirichella - 0001222371"),

  // Optional Styling (for more and explanation of options take a look at the typst universe)
  ratio: 16/9,
  layout: "medium",
  title-color: blue.darken(70%),
  toc: true,
)

= Proposta di progetto

== Obiettivi
Lo scopo è quello di trasformare una matrice quadrata di numeri reali in una matrice binaria basata sulla media locale di ogni elemento.

- *Matrice di Input ($A$):* Quadrata di dimensione $N times N$, con $N >= 2000$. Contiene valori reali.
- *Matrice di Output ($T$):* Quadrata di dimensione $N times N$. Contiene valori interi binari ${0, 1}$.

#v(1em)

// Griglia per affiancare matrice e spiegazione
#grid(
  columns: (2fr, 1.4fr), 
  gutter: 2em,
  
  align(center + horizon)[
  / Intorno ($I_(i j)$): Per ogni elemento $a_(i,j)$ della matrice $A$, si definisce un *intorno* come la sottomatrice $3 times 3$ centrata nell'elemento stesso:
    ],

    align(horizon)[
      #align(center)[
        #image("drawings/hpc-proj-intorno.drawio.png", width: 70%)
    ]
  ]
)

Il valore di ogni elemento $t_(i,j)$ della matrice risultante $T$ viene determinato seguendo questi passaggi:

1. *Media dell'intorno ($m_(i,j)$):* 
- Si calcola la media aritmetica di tutti i 9 elementi appartenenti all'intorno $I_(i j)$ con la formula: #align(center)[$ m_(i,j) = 1/9 sum_(x=i-1)^(i+1) sum_(y=j-1)^(j+1) a_(x,y) $] 

2. *Sogliatura binaria:*
   - Se $a_(i,j) > m_(i,j)$ allora $t_(i,j) = 1$
   - Se $a_(i,j) <= m_(i,j)$ allora $t_(i,j) = 0$

3. *Complessità:* 

  - Data la dimensione $N >= 2000$, l'algoritmo deve processare almeno $4 times 10^6$ elementi, rendendo l'ottimizzazione o la parallelizzazione rilevante.

= Implementazione Seriale
== Considerazioni generali
L'algoritmo seriale analizza ogni cella $A(i, j)$, calcola la media dei vicini (inclusa la cella stessa) e assegna un valore binario basato sul confronto tra il valore centrale e la media locale.

```c
// Accesso alla matrice tramite puntatore a array
int (*A)[n_size] = (int (*)[n_size])A_raw; 
int (*T)[n_size] = (int (*)[n_size])T_raw; 

/* Calcolo della Binarizzazione */
for (int i = 0; i < n_size; i++) {
    for (int j = 0; j < n_size; j++) {
        sum = 0, count = 0;

        // definizione dei confini (3x3 con "fallback" dei bordi)
        int zmin = (i > 0) ? i - 1 : i;
        int zmax = (i < n_size - 1) ? i + 1 : i;
        int wmin = (j > 0) ? j - 1 : j;
        int wmax = (j < n_size - 1) ? j + 1 : j;

        for (int z = zmin; z <= zmax; z++) {
            for (int w = wmin; w <= wmax; w++) {
                sum += A[z][w];
                count++;
            }
        }
        
        // calcolo sogliatura binaria
        T[i][j] = (A[i][j] * count > sum) ? 1 : 0;
    }
}
```

Si noti la presenza di due leggere ottimizzazioni:
1. Gestione dei bordi tramite "Ghost Cells".

2. Uso della formula ($A[i][j] * "count" > "sum"$) al posto della divisione ($"sum""/""count"$)


= Implementazione Parallela: MPI
== Considerazioni generali


Essendo la matrice "distribuita" tra processi differenti, un processo necessita di dati appartenenti alla memoria di un altro per calcolare i valori sui bordi del proprio intorno.

Prima di affrontare il problema, è necessario comprendere il modo in cui la matrice viene linearizzata in memoria:

#v(1em)

#grid(
  columns: (1fr, 1.4fr), 
  gutter: 2em,
  
  align(center + horizon)[
    #set math.mat(column-gap: 0.8em)
    $ A = mat(
      a_(11), a_(12), dots, a_(1n);
      a_(21), a_(22), dots, a_(2n);
      dots.v, dots.v, dots.down, dots.v;
      a_(n 1), a_(n 2), dots, a_(n n);
    ) $
    #text(size: 0.8em, fill: gray)[Marice $n times n$]
  ],

  align(horizon)[
    *Memorizzazione Row-Major:* \
    La matrice viene "appiattita" riga dopo riga. Righe adiacenti logicamente sono rappresentate come contigue in memoria.

    *Rappresentazione in memoria:* \
    #block(
      fill: rgb("#f0f0f0"),
      inset: 5pt,
      radius: 4pt,
      stroke: gray.lighten(50%)
    )[
      $ [ [a_(11), dots, a_(1n)], [a_(21), dots, a_(2n)], dots, [a_(n 1), dots, a_(n n)] ] $
    ]
  ]
)

Perciò, se abbiamo una matrice $n times n$, l’elemento in posizione $(i, j)$ può essere acceduto linearmente come: `A[i * n_size + j]`.

== Distribuzione del carico di lavoro

Poiché la dimensione della matrice $N$ (`n_size`) non è sempre un multiplo esatto del numero di processi $P$ (`size`), è necessaria una strategia di partizionamento per garantire una distribuzione equa del carico.

Il nodo Master adotta una logica basata sulla divisione intera e sul resto:

#grid(
  columns: (1fr, 1fr),
  gutter: 15pt,
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`base_rows`*: Ogni processo riceve almeno "`n_size / size`" righe.
  ],
  block(fill: rgb("#f0f0f0"), inset: 10pt, radius: 4pt, width: 100%)[
    *`extra_rows`:* Le righe rimanenti ("`n_size % size`") vengono ridistribuite.
  ]
)

Per inviare  blocchi di dimensione variabile (primitiva `MPI_Scatterv`), vengono popolate due strutture dati fondamentali:

- *#raw("sendcounts")*: Memorizza il numero totale di elementi assegnati a ogni processo.

- *#raw("senddispls")*: Definisce l'indice di partenza (displacement) di ogni blocco all'interno della memoria del Master. Si calcola come somma cumulativa degli elementi assegnati ai processi precedenti: #raw("senddispls[p]") $= sum_(i=0)^(p-1) "sendcounts"[i]$. 

/ Esempio: Matrice: $5 times 5$, Numero di processi: $4$

#set text(size: 10pt)
#grid(
  columns: (1fr, 1fr),
  column-gutter: 20pt,
  [
    #align(left)[
      *1. MPI_Bcast* \
      Il master comunica a tutti quanto dovranno allocare (`sendcounts` e `senddispls`).
      #v(5pt)
      #image("drawings/hpc-proj-bcast.drawio.png", width: 67%)
    ]
  ],
  
  [
    #align(left)[
      *2. MPI_Scatterv* \
      Il master, dopo l'allocazione della matrice, invia i segmenti di dati reali alle memorie locali dei processi.
      #v(5pt)
      #image("drawings/hpc-proj-scatterv.drawio.png", width: 90%)
    ]
  ]
)

== Allocazione e scambio delle Ghost Rows

Ogni processo alloca la porzione di righe indicata  dal master (`my_rows = sendcounts[my_rank] / n_size`) più due righe aggiuntive:
- *Upper Ghost Row:* Riceve l'ultima riga del processo precedente ($P_(k-1)$) se esiste.
- *Lower Ghost Row:* Riceve la prima riga del processo successivo ($P_(k+1)$) se esiste.

#align(center)[
  #image("drawings/hpc-proj-matrix-scatter.drawio.png", width:72%)
]


Successivamente, ogni processo determina i processi adiacenti e il numero di Ghost Rows da allocare in funzione della propria posizione nel dominio globale:

```c
    int up   = (my_rank > 0)        ? my_rank - 1 : MPI_PROC_NULL;
    int down = (my_rank < size - 1) ? my_rank + 1 : MPI_PROC_NULL;
    int total_rows = my_rows + 2; // sempre due per semplicità
```

Successivamente, copia i dati della propria porzione della matrice all'interno della matrice con le Ghost Rows allocate: 

// Griglia per affiancare matrice e spiegazione
#grid(
  columns: (1fr, 2fr), 
  gutter: 1em,
  
  align(center + horizon)[
     #image("drawings/hpc-proj-A_plus_ghosts.drawio.png")
  ],
  
  align(horizon)[
    ```c
    int *my_A_plus_ghosts = calloc(total_rows * n_size, sizeof(int));
    
    // puntatori ad interi per le "sottoparti" di my_A
    int *upper_ghost = my_A_plus_ghosts;
    int *local_data  = my_A_plus_ghosts + n_size; 
    int *lower_ghost = my_A_plus_ghosts + (my_rows + 1) * n_size;

  ```
  ]
)


Per consentire l'invio e la ricezione delle Ghost Rows, si utilizzano le primitive di comunicazione punto-a-punto:

- La primitiva `MPI_Recv` è sempre bloccante.
- La primitiva `MPI_Ssend` implementa una comunicazione sincrona di tipo *rendez-vous*.

Successivamente, ogni processo (tranne il primo) invia la propria riga di bordo superiore al vicino `up` (in 

#grid(
  columns: (2.2fr, .7fr),
  column-gutter: 10pt,
  align: horizon,
  [
    ```c
if (my_rank > 0 && my_rows > 0) {
  MPI_Ssend(local_data, n_size, MPI_INT, up, 100, MPI_COMM_WORLD);
  
  MPI_Recv(upper_ghost, n_size, MPI_INT, up, 200, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-2.drawio.png", width: 100%)]
)

Successivamente, si calcola l'indice di memoria dove inizia l'ultima riga locale per l'invio al processo sottostante:

```c
int send_down_offset = (my_rows > 0 ? (my_rows - 1) * n_size : 0);
```

L'invio tramite `Ssend` sincronizza i processi: una volta completata la trasmissione della riga di confine, il processo viene "sbloccato" per la ricezione dal nodo superiore della *Ghost Row* necessaria.

#grid(
  columns: (2.7fr, 1.1fr),
  column-gutter: 10pt,
  align: horizon,
  [
    ```c
    if (my_rank < size - 1 && my_rows > 0) {
        if (sendcounts[down] > 0) {
            MPI_Recv(lower_ghost, n_size, MPI_INT, down, 100, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            MPI_Ssend(local_data + send_down_offset, n_size, MPI_INT, down, 200, MPI_COMM_WORLD);
        }
    }
    ```
  ],
  [ #image("drawings/hpc-proj-ssend-recv-1.drawio.png", width: 70%)]
)

== Calcolo elementi dell'intorno e sogliatura

Dopo lo scambio dei dati, ogni processo itera sulla propria porzione di matrice. Si ricorda che:

  #table(
    columns: (2.3fr, 2.3fr),
    inset: 3pt,
    stroke: gray.lighten(60%),
  
    [Indice], [Contenuto],
    [`my_A_plus_ghosts[0]`],                          [Ghost Row superiore],
    [`my_A_plus_ghosts[1] … [rows_per_proc]`],          [Righe di dati locali],
    [`my_A_plus_ghosts[rows_per_proc + 1]`],          [Ghost Row inferiore],
  )
  
  Per compensare la presenza delle Ghost Rows, l'indice locale deve quindi essere traslato verticalmente:
  ```c
  int zmin = (i == 0 && my_rank == 0) ? 0 : -1;
  int zmax = (i == my_rows - 1 && my_rank == last_rank_with_data) ? 0 : 1;
  ```
  E orizzontalmente:
  ```c
  int wmin = (j > 0) ? -1 : 0;
  int wmax = (j < n_size - 1) ? 1 : 0;
  ```


  #grid(
  columns: (2.2fr, 1fr), 
  gutter: 1em,
  
  align(left + horizon)[
    Nel calcolo della soglia locale, l'indice di riga viene calcolato come $i+1+"dz"$ per saltare correttamente la ghost row superiore.
      ```c
for (int dz = zmin; dz <= zmax; dz++) {
  int rowIndex = i + 1 + dz;
  
  for (int dw = wmin; dw <= wmax; dw++) {
    int colIndex = j + dw;
    
    sum += my_A_plus_ghosts[rowIndex * n_size +
    colIndex];
    
    count++;
  }
}

 my_T[i * n_size + j] =  //soglia binaria...
  ```
  ],
  align(horizon)[
     #image("drawings/hpc-proj-matrix-for-cycle.drawio.png")
  ]
)

== Liberazione memoria allocata e raccolta risultati

Si libera la memoria allocata per la matrice localecon le ghost rows:

```c
free(my_A_plus_ghosts);
```
Il nodo Master invoca la primitiva `MPI_Gatherv` per collezionare i risultati parziali:
```c
MPI_Gatherv(
    my_T, (my_rows * n_size), MPI_INT,      // send buffer
    T_raw, sendcounts, senddispls, MPI_INT, // recv buffers
    0, MPI_COMM_WORLD
);
```

Ed infine, tutti i nodi liberano la propria porzione di matrice e le strutture dati allocate in precedenza 
```c
if (my_rows > 0) 
  { free(my_A); free(my_T); }
free(sendcounts); free(senddispls);
```

= Implementazione Parallela: OpenMP
== Considerazioni generali

= Benchmark
== MPI
== Tabella Strong Scaling
//#generate-scaling-table(mpi_strong, is-weak: false)
#mpi_table(mpi_strong, mode: "strong")


== Tabella Weak Scaling (Gustafson)
#mpi_table(mpi_weak, mode: "weak", r: 0.9)
//#generate-scaling-table(mpi_weak, is-weak: true)

#align(center)[

#image("plot/mpi_strong_comparison.png")
]

#align(center)[

#image("plot/mpi_weak_comparison.png")
]

== OpenMP

= Conclusioni