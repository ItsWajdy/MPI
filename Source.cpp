#include <iostream>
#include <Windows.h>
#include <ctime>
#include "mpi.h"

using namespace std;

const int N = 10;
const int MAX_CHILDREN = 25;

int main(int argc, char* argv[]) {
	MPI_Init(&argc, &argv);

	int numTasks = 0;
	int myRank = 0;

	MPI_Comm_size(MPI_COMM_WORLD, &numTasks);
	MPI_Comm_rank(MPI_COMM_WORLD, &myRank);

	int districtTag = 1;
	int processTag = 2;

	MPI_Status status;

	if (myRank == 0) {
		int districts = 0;

		// Number of representitives and average wait time in each district
		int reps[N];
		double avgTime[N];

		// Number of pharmacies and cliniques in each district
		int pharmacies[N];
		int cliniques[N];
		int total[N];

		cout << "Number of Districts: ";
		cin >> districts;

		for (int i = 1; i <= districts; i++) {
			cout << endl << "Number of Pharmacies in District " << i << ": ";
			cin >> pharmacies[i];
			cout << "Number of Cliniques in District " << i << ": ";
			cin >> cliniques[i];

			cout << "Number of Representitives in District " << i << ": ";
			cin >> reps[i];

			total[i] = pharmacies[i] + cliniques[i];

			cout << "Average Time Per Distributing Process in District " << i << ": ";
			cin >> avgTime[i];
		}

		clock_t start = clock();
		int processIdx = N + 1;
		for (int i = 1; i <= districts; i++) {
			for (int j = 0; j < reps[i]; j++) {
				MPI_Send(&i, 1, MPI_INT, processIdx, processTag, MPI_COMM_WORLD);
				processIdx++;
			}
		}

		int terminate = -1;
		for (; processIdx < numTasks; processIdx++) MPI_Send(&terminate, 1, MPI_INT, processIdx, processTag, MPI_COMM_WORLD);

		// Send ranks of children of each district to it
		processIdx = N + 1;
		for (int i = 1; i <= districts; i++) {
			MPI_Send(&reps[i], 1, MPI_INT, i, districtTag, MPI_COMM_WORLD);
			for (int j = 0; j < reps[i]; j++) {
				MPI_Send(&processIdx, 1, MPI_INT, i, districtTag, MPI_COMM_WORLD);
				processIdx++;
			}

			MPI_Send(&avgTime[i], 1, MPI_DOUBLE, i, districtTag, MPI_COMM_WORLD);
			MPI_Send(&total[i], 1, MPI_INT, i, districtTag, MPI_COMM_WORLD);
		}


		// Terminate unwanted processes
		for (int i = districts + 1; i <= N; i++) {
			MPI_Send(&terminate, 1, MPI_INT, i, districtTag, MPI_COMM_WORLD);
		}


		char message = 'q';
		for (int i = 1; i <= districts; i++) {
			MPI_Recv(&message, 1, MPI_CHAR, i, districtTag, MPI_COMM_WORLD, &status);
		}

		cout << "Parallel Time: " << (clock() - start) / (double)(CLOCKS_PER_SEC / 1000) << "ms" << endl;
	}
	else if (myRank >= 1 && myRank <= N) {       // District
		int reps = 0;
		int childrenRanks[MAX_CHILDREN];
		double waitTime = 0;
		int totalProcesses = 0;

		MPI_Status status;

		MPI_Recv(&reps, 1, MPI_INT, 0, districtTag, MPI_COMM_WORLD, &status);
		if (reps < 0) { MPI_Finalize(); return 0; }
		for (int i = 0; i < reps; i++) MPI_Recv(&childrenRanks[i], 1, MPI_INT, 0, districtTag, MPI_COMM_WORLD, &status);
		MPI_Recv(&waitTime, 1, MPI_DOUBLE, 0, districtTag, MPI_COMM_WORLD, &status);
		MPI_Recv(&totalProcesses, 1, MPI_INT, 0, districtTag, MPI_COMM_WORLD, &status);

		int served = 0;
		char message;

		while (served < totalProcesses) {
			for (int i = childrenRanks[0]; i < childrenRanks[0] + reps && served + i - childrenRanks[0] < totalProcesses; i++) {
				MPI_Send(&waitTime, 1, MPI_DOUBLE, i, processTag, MPI_COMM_WORLD);
			}
			for (int i = childrenRanks[0]; i < childrenRanks[0] + reps && served + i - childrenRanks[0] < totalProcesses; i++) {
				MPI_Recv(&message, 1, MPI_CHAR, i, processTag, MPI_COMM_WORLD, &status);
			}
			served += reps;
		}

		double terminate = -1;
		for (int i = childrenRanks[0]; i < childrenRanks[0] + reps; i++) {
			MPI_Send(&terminate, 1, MPI_DOUBLE, i, processTag, MPI_COMM_WORLD);
		}

		char doneMessage = '0';
		MPI_Send(&doneMessage, 1, MPI_CHAR, 0, districtTag, MPI_COMM_WORLD);
	}
	else {       // Process
		int parentDistrict = 0;
		MPI_Recv(&parentDistrict, 1, MPI_INT, 0, processTag, MPI_COMM_WORLD, &status);
		if (parentDistrict < 0) { MPI_Finalize(); return 0; }

		while (true) {
			double waitTime = 0;
			char doneMessage = '0';

			MPI_Status status;

			MPI_Recv(&waitTime, 1, MPI_DOUBLE, parentDistrict, processTag, MPI_COMM_WORLD, &status);
			if (waitTime < 0) break;

			Sleep(1000 * waitTime);
			MPI_Send(&doneMessage, 1, MPI_CHAR, parentDistrict, processTag, MPI_COMM_WORLD);
		}
	}

	MPI_Finalize();
	return 0;
}