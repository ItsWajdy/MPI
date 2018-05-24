#include <iostream>
#include <Windows.h>
#include <ctime>
#include "mpi.h"

using namespace std;

void serve(int representitives, int servees, double avgTime, int tag) {
	MPI_Status status;
	int served = 0;
	while (served < servees) {
		char message;
		for (int i = 1; i <= representitives && served + i <= servees; i++) MPI_Send(&avgTime, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
		for (int i = 1; i <= representitives && served + i <= servees; i++) MPI_Recv(&message, 1, MPI_CHAR, i, tag, MPI_COMM_WORLD, &status);
		served += representitives;
	}
}

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int numTasks = -1;
	int myRank = -1;

	MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	int tag = 1;
	MPI_Status status;

	if (myRank == 0) {
		int pharmacies, cliniques;
		cout << "Number of Pharmacies: ";
		cin >> pharmacies;
		cout << "Number of Cliniques: ";
		cin >> cliniques;

		int representitives;
		double avgTime;
		cout << "Number of Representitives: ";
		cin >> representitives;
		cout << "Average Time of One Process: ";
		cin >> avgTime;

		int cliniquesServed = 0;

		clock_t start = clock();
		serve(representitives, pharmacies, avgTime, tag);
		serve(representitives, cliniques, avgTime, tag);
		cout << "Parallel Time: " << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << "ms" << endl;

		start = clock();
		serve(1, pharmacies, avgTime, tag);
		serve(1, cliniques, avgTime, tag);

		//for (int i = 0; i < pharmacies; i++) Sleep(avgTime * 1000);
		//for (int i = 0; i < cliniques; i++) Sleep(avgTime * 1000);
		cout << "Sequential Time: " << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << "ms" << endl;

		double terminateFlag = -1;
		for (int i = 1; i < numTasks; i++) MPI_Send(&terminateFlag, 1, MPI_DOUBLE, i, tag, MPI_COMM_WORLD);
	}
	else {
		while (true) {
			double waitTime = 0;
			char message = 'x';

			MPI_Recv(&waitTime, 1, MPI_DOUBLE, 0, tag, MPI_COMM_WORLD, &status);
			if (waitTime < 0) break;

			Sleep(waitTime * 1000);
			MPI_Send(&message, 1, MPI_CHAR, 0, tag, MPI_COMM_WORLD);
		}
	}

	MPI_Finalize();
}