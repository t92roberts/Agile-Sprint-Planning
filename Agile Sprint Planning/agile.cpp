#include <ilcplex/ilocplex.h>
#include <vector>
#include <string>
#include <algorithm>
#include <set>
#include <tuple>
ILOSTLBEGIN

using namespace std;

class Story {
public:
	int storyNumber, businessValue, storyPoints;
	vector<int> dependencies;

	Story() {};

	Story(int storyNumber, int businessValue, int storyPoints) {
		this->storyNumber = storyNumber;
		this->businessValue = businessValue;
		this->storyPoints = storyPoints;
	}

	Story(int storyNumber, int businessValue, int storyPoints, vector<int> dependencies) {
		this->storyNumber = storyNumber;
		this->businessValue = businessValue;
		this->storyPoints = storyPoints;
		this->dependencies = dependencies;
	}

	string printDependencies() {
		if (this->dependencies.size() > 0) {
			string dependenciesString = "";

			for (int i = 0; i < this->dependencies.size(); ++i) {
				if (i == 0) {
					dependenciesString += "Story " + to_string(this->dependencies[i]);
				}
				else {
					dependenciesString += ", Story " + to_string(this->dependencies[i]);
				}
			}

			return dependenciesString;
		}
		else {
			return "None";
		}
	}

	string toString() {
		return "Story " + to_string(storyNumber)
			+ " (business value: " + to_string(businessValue)
			+ ", story points: " + to_string(storyPoints)
			+ ", dependencies: " + printDependencies() + ")";
	}
};

class Sprint {
public:
	int sprintNumber, sprintCapacity, sprintBonus;

	Sprint() {};

	Sprint(int num, int cap, int bonus) {
		this->sprintNumber = num;
		this->sprintCapacity = cap;
		this->sprintBonus = bonus;
	}

	string toString() {
		return ">> Sprint " + to_string(sprintNumber) +
			" (capacity: " + to_string(sprintCapacity) +
			", bonus: " + to_string(sprintBonus) + ")";
	}
};

class Point {
public:
	int x, y;

	Point() {};

	Point(int newX, int newY) {
		this->x = newX;
		this->y = newY;
	}

	// Returns true if the first point is before the second point
	bool operator<(Point point) {
		if (this->x < point.x)
			return true;
		else if (this->x > point.x)
			return false;
		else if (this->y < point.y)
			return true;
		else
			return false;
	}
};

// Returns a random int between min and max (both inclusive) using a uniform distribution
int randomInt(int min, int max) {
	return rand() % (max - min + 1) + min;
}

// Returns a random position in the input vector according to the given probability distribution of getting each position
int randomIntDiscreteDistribution(vector<double> probabilities) {
	// Randomly generated percentage
	double randomPercentage = (double)(rand() % 1024) / 1024;

	// Threshold representing the upper limit of each probability band
	double threshold = probabilities[0];

	for (int i = 0; i < probabilities.size(); ++i) {
		// If the random percentage is within this vector position's probability threshold, return the position
		if (randomPercentage < threshold)
			return i;
		else
			// If not, extend the threshold to the next probability band
			threshold += probabilities[i + 1];
	}
}

// Uses the parametric equation of a geometric sequence to return a vector of doubles
vector<double> geometricSequence(double a, double r, double n) {
	vector<double> sequence;

	for (int i = 0; i < n; ++i) {
		sequence.push_back(a * pow(r, i));
	}

	return sequence;
}

vector<string> splitString(const string& s, char delimiter) {
	vector<string> tokens;
	string token;
	istringstream tokenStream(s);

	while (getline(tokenStream, token, delimiter)) {
		tokens.push_back(token);
	}

	return tokens;
}

int main(int argc, char* argv[]) {
	// Seed the random number generator
	srand(time(NULL));

	vector<Story> storyData;
	string storyDataFileName;

	// Holds the data about each sprint
	vector<Sprint> sprintData;
	string sprintDataFileName;

	switch (argc) {
	case 3:
		storyDataFileName = argv[1];
		sprintDataFileName = argv[2];
		break;
	default:
		exit(0);
	}

	// Load story data into objects //////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	string line;
	ifstream storiesFile(storyDataFileName);

	getline(storiesFile, line); // Skip column headers

	vector<pair<int, string>> dependenciesStrings;

	if (storiesFile.is_open()) {
		while (getline(storiesFile, line)) {
			vector<string> splitLine = splitString(line, ',');

			int storyNumber = stoi(splitLine[0]);
			int businessValue = stoi(splitLine[1]);
			int storyPoints = stoi(splitLine[2]);

			storyData.push_back(Story(storyNumber, businessValue, storyPoints));

			if (splitLine.size() == 4) {
				string dependencyString = splitLine[3];

				vector<string> splitDependencies = splitString(dependencyString, ';');

				for (string dependeeString : splitDependencies) {
					storyData[storyNumber].dependencies.push_back(stoi(dependeeString));
				}
			}
		}
	}
	else {
		cout << "Cannot open story data file" << endl;
		exit(0);
	}

	storiesFile.close();

	// Load sprint data into objects /////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////

	ifstream sprintsFile(sprintDataFileName);

	getline(sprintsFile, line); // Skip column headers

	if (sprintsFile.is_open()) {
		while (getline(sprintsFile, line)) {
			vector<string> splitLine = splitString(line, ',');

			int sprintNumber = stoi(splitLine[0]);
			int sprintCapacity = stoi(splitLine[1]);
			int sprintBonus = stoi(splitLine[2]);

			sprintData.push_back(Sprint(sprintNumber, sprintCapacity, sprintBonus));
		}
	}
	else {
		cout << "Cannot open sprint data file" << endl;
		exit(0);
	}

	sprintsFile.close();

	int numberOfStories = storyData.size();
	int numberOfSprints = sprintData.size();

	IloEnv env;

	try {
		IloModel model(env);

		// 2D array of Boolean decision variables (roadmap[i][j] == 1 means user story j is taken in sprint i)
		IloArray<IloBoolVarArray> roadmap(env, numberOfSprints);

		// Total business value delivered (across the whole roadmap)
		IloNumExpr deliveredValue(env, 0);

		for (int i = 0; i < numberOfSprints; ++i) {
			// Sum of the story points taken in the sprint i
			IloNumExpr storyPointsTaken(env, 0);

			// Sprint i is represented by an array of Boolean decision variables
			roadmap[i] = IloBoolVarArray(env, numberOfStories);

			for (int j = 0; j < numberOfStories; ++j) {
				// Create a Boolean decision variable
				roadmap[i][j] = IloBoolVar(env);

				// Add business value (including sprint bonus), if story is taken in this sprint
				deliveredValue += storyData[j].businessValue * sprintData[i].sprintBonus * roadmap[i][j];

				// Add story points, if story j is taken in sprint i
				storyPointsTaken += storyData[j].storyPoints * roadmap[i][j];
			}

			// Add constraint (per sprint): sum of story points taken in sprint i <= sprint i's capacity
			model.add(storyPointsTaken <= sprintData[i].sprintCapacity);
		}

		// Story is only assiged to one (or no) sprint
		for (int j = 0; j < numberOfStories; ++j) {
			// How many times story j is assigned in sprint i
			IloNumExpr numberOfTimesStoryIsUsed(env, 0);

			for (int i = 0; i < numberOfSprints; ++i) {
				numberOfTimesStoryIsUsed += roadmap[i][j];
			}

			// Story j can be included in sprint i <= 1 times
			model.add(numberOfTimesStoryIsUsed <= 1);
		}

		// Add dependency constraints
		for (int i = 0; i < numberOfSprints; ++i) {
			for (int j = 0; j < numberOfStories; ++j) {
				// How many dependencies the story has
				int numberOfDependencies = storyData[j].dependencies.size();

				for (int d = 0; d < numberOfDependencies; ++d) {
					// The story number of the dependee story
					int storyToCheck = storyData[j].dependencies[d];

					// How many times the dependee story has been assigned before to this sprint
					IloNumExpr numberOfTimesDependeesPreAssigned(env, 0);

					// Count how many times the dependee has been included between the first sprint and the current sprint
					for (int sprintLookback = 0; sprintLookback < i; ++sprintLookback) {
						numberOfTimesDependeesPreAssigned += roadmap[sprintLookback][storyToCheck];
					}

					// roadmap[i][j] == 1 means that dependent story j is taken in sprint i, so dependee story d must appear in previous roadmap
					// roadmap[i][j] == 0 means that dependent story j is not taken in sprint i, so dependee story d may or may not appear in previous roadmap
					model.add(IloIfThen(env, roadmap[i][j] == 1, numberOfTimesDependeesPreAssigned == 1));

					// NOTE - is numberOfTimesDependeesPreAssigned == 1 (per dependee, per story) better than numberOfTimesDependeesPreAssigned == numberOfDependencies (per story)?
					// i.e. is it better to have more or fewer constraints that check the same thing?
				}
			}
		}

		// Objective function
		model.add(IloMaximize(env, deliveredValue));

		IloCplex cplex(model);

		if (cplex.solve()) {
			int totalStoriesDelivered = 0;
			int totalBusinessValueDelivered = 0;
			int totalStoryPointsDelivered = 0;

			if (true) {
				/*cout << endl << "Product backlog: " << endl << endl;

				for (int i = 0; i < storyData.size(); ++i) {
					cout << storyData[i].toString() << endl;
				}

				cout << endl << "Available sprints: " << endl << endl;

				for (int i = 0; i < sprintData.size(); ++i) {
					cout << sprintData[i].toString() << endl;
				}*/

				cout << endl << "---------------------------------------------------------" << endl << endl;

				cout << "Sprint plan:" << endl << endl;

				// Output the solution
				for (int i = 0; i < numberOfSprints; ++i) {
					// Sprint information
					cout << sprintData[i].toString() << endl;
					int businessValueDelivered = 0;
					int storyPointedDelivered = 0;

					for (int j = 0; j < numberOfStories; ++j) {
						// If the story was taken in this sprint, print the story's information
						if (cplex.getValue(roadmap[i][j]) == 1) {
							businessValueDelivered += storyData[j].businessValue;
							storyPointedDelivered += storyData[j].storyPoints;

							// Running totals for how much the roadmap delivered overall
							totalBusinessValueDelivered += storyData[j].businessValue;
							totalStoryPointsDelivered += storyData[j].storyPoints;

							cout << "\t" << storyData[j].toString() << endl;
						}
					}

					// What was delivered in the sprint
					cout << "-- [Value: " << to_string(businessValueDelivered)
						<< " (weighted business value: " << to_string(businessValueDelivered * sprintData[i].sprintBonus) << "), "
						<< "story points: " << to_string(storyPointedDelivered) << "]" << endl << endl;
				}
			}

			cout << endl << "Solution status: " << cplex.getStatus() << endl;
			cout << "Total weighted business value: " << cplex.getObjValue() << endl << endl;
		}
		else {
			cout << " No solution found" << endl;
		}

	}
	catch (IloAlgorithm::CannotExtractException &e) {
		IloExtractableArray const &failed = e.getExtractables();
		std::cerr << "Failed to extract:" << std::endl;
		for (int i = 0; i < failed.getSize(); ++i)
			std::cerr << failed[i] << std::endl;
	}
	catch (IloException& e) {
		cerr << "Concert exception caught: " << e.getMessage() << endl;
	}
	catch (...) {
		cerr << "Unknown exception caught" << endl;
	}

	env.end();

	return 0;
}