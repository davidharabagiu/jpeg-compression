Ruleaza pe release! Pe debug se misca foarte greu la input mare.

Programul primeste 3-4 argumente:

1. c (pentru compresie) / d (pentru decompresie)
2. numele fisierului sursa
3. numele fisierului destinatie
4. optional, daca se face compresie, se poate specifica calitatea imaginii compresate, daca nu se specifica acest argument, se alege valoarea implicita 32; acest numar reprezinta numarul de coeficienti din fiecare partitie 8x8 ce se va scrie in fisier

Va face compresia si va salva rezultatul intr-un fisier binar cu urmatorul format:

no_of_rows [4 bytes]

no_of_colums [4 bytes]

quality [4 bytes]

partition_0 [(quality) bytes]:
	coeff_0 [1 byte]
	coeff_1 [1 byte]
	...
	coeff_(quality - 1) [1 byte]

partition_1 [(quality) bytes]:
	coeff_0 [1 byte]
	coeff_1 [1 byte]
	...
	coeff_(quality - 1) [1 byte]

...

partition_(rows x cols / 64 - 1) [(quality) bytes]:
	...

Decompresia accepta doar acest format.