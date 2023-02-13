#include <Windows.h>
#include <iostream>
#include <fstream>
#include <utility>
#include <vector>
#include <string>
#include <regex>
#include <thread>

//Univers.txt -i -1 -o output.txt
const bool AliveCell = 1;
const bool EmptyOrDead = 0;

const bool OfflineGameMode = true;
const bool OnlineGameMode = false;

const char ac = 1;

//������ � ������������ � ���. �����\�� �����
class Cell {
public:
	int x, y;
	bool isAlive;
	Cell(int _x, int _y, bool _isAlive = true) { x = _x, y = _y, isAlive = _isAlive; }		//�����������
	~Cell() = default;																		//����������
};


//������ � ������ ��������� �� ������
class Messages
{
public:
	void showWarning(std::string message)
	{
		std::cout << "Warning!\n" + message + '\n';
	}
	//������� ��������� � ���������
	void showReference()
	{
		std::cout << 
			"Simple implementation of Game of life. \n\
			\n\
			Universe input file format: \n\
			1. #N Version \n\
			2. #S wigth, heigth \n\
			3. #R #Bx / Sy - birth and survival rule, x and y can be any number from{ 0..8 }\n\
			4. Cells coordinates : x, y in range of width and height.\n\
			\n\
			Commands: \n\
			1. help - show game reference\n\
			2. dump<filename> -save current universe state\n\
			3. exit - end the game\n\
			4. tick<n> -count game state after n iteration\n\
			\n\
			____________________________________________________________________\n\
			\n\
			������� ���������� ���� '�����'\n\
			\n\
			������ �������� �����:\n\
			1. #N ������\n\
			2. #S ������, ������ ����\n\
			3. #R #Bx #Sy ������� �������� � ���������, � � � - ����� �� 0 �� 9\n\
			4. ���������� ��������� ������ � ��������� ������ � ������ ����\n\
			\n\
			��������� �������:\n\
			1. help - ���������� ������� �����\n\
			2. dump<filename> - ��������� ������ ��� � ����\n\
			3. exit - ����� ����\n\
			4. tick<n> - �������� ����� ���� �� n ��������\n\
			\n\
			";
	}

	void errUnknownCommand()
	{
		std::cout << "Unknown command!\n";
	}

	void errNoInputFile()
	{
		std::cout << "There's no such file!\n";
	}

	void done()
	{
		std::cout << "Done!\n";
	}
};

//����� ��������� ����
class GameState {
public:

	std::string birthRules;
	std::string survivalRules;
	int heigthOfField;
	int wigthOfField;

	std::string fileName;
	std::string universeName;
	std::vector <Cell> cellsCoordinates;
};

//����� ��� ������ �����
class GameLoader : public GameState
{
public:
	//���� ���� �� �����
	GameLoader()
	{
		Messages msg;
		msg.showWarning("There's no input file. One of the default universes will be loaded");
		loadDefaultUniverse();
	}

	//���� ���� ����� �� �����
	GameLoader(std::wstring fileName)
	{
		Messages msg;
		if (!this->parseInputFile(fileName))
		{
			msg.showWarning("There's no input file. One of the default universes will be loaded");
		}
	}

private:
	//���� �� �����-�� ������� ���� �� ��������� �������������
	void setDefaultSettings()
	{
		this->birthRules = "2";
		this->survivalRules = "23";
		this->heigthOfField = 30;
		this->wigthOfField = 30;
	}


	//�������� ��������� ��������� �� �����������
	void loadDefaultUniverse()
	{
		WIN32_FIND_DATAW wfd;
		HANDLE const hFind = FindFirstFileW(L"C:\\Users\\slava\\source\\repos\\Game_of_Life\\Default_universes\\*", &wfd);
		std::vector<std::wstring> allUniverses;

		if (INVALID_HANDLE_VALUE != hFind)
		{
			do
			{
				allUniverses.push_back(wfd.cFileName);
			} while (NULL != FindNextFileW(hFind, &wfd));

			//Remove useless directories
			allUniverses.erase(allUniverses.begin());
			allUniverses.erase(allUniverses.begin());

			FindClose(hFind);
		}

		std::srand(std::time(nullptr));
		int currentUniverse = std::rand() % allUniverses.size();

		this->parseInputFile(L"C:\\Users\\slava\\source\\repos\\Game_of_Life\\Default_universes\\" + allUniverses[currentUniverse]);
	}

	//��������, ��� �������� � ��������
	int parseInputFile(std::wstring inputFile)
	{
		//In case there's no input file set a default settings
		setDefaultSettings();

		std::ifstream fin(inputFile.c_str());
		if (!fin.is_open())
		{
			loadDefaultUniverse();
			return this->failed_to_open;
		}

		std::regex rulesRegex("(#R )(B[0-9]+\/S[0-9]+)");
		std::regex sizeRegex("[#S ]([0-9]+) ([0-9]+)");
		std::regex universeNameRegex("[#N ]([A-Za-z]*)");
		std::regex digits("[0-9]+");
		std::regex letters("[A-Za-z ]+");
		std::smatch s;

		//�����
		char temp[256];

		//��� ���������
		fin.getline(temp, 256);
		if (std::regex_search(temp, universeNameRegex))
		{
			this->universeName = temp;
			this->universeName.erase(this->universeName.find("#N "), 3);
		}

		//������� ���������
		fin.getline(temp, 256);
		if (std::regex_search(temp, rulesRegex))
		{
			std::string str(temp);
			auto iter = std::sregex_iterator(str.begin(), str.end(), digits);
			this->birthRules = (*iter).str();
			this->survivalRules = (*(++iter)).str();
		}

		//������� ���� ���������
		fin.getline(temp, 256);
		if (std::regex_search(temp, sizeRegex))
		{
			std::string str(temp);
			auto iter = std::sregex_iterator(str.begin(), str.end(), digits);
			this->wigthOfField = std::stol((*iter).str());
			this->heigthOfField = std::stol((*(++iter)).str());
		}

		//���������� ������
		int x, y;
		while (!fin.eof()) 
		{
			fin >> x >> y;
			this->cellsCoordinates.push_back(Cell(x, y, true));
		}
		return file_success;
	}

	static const int failed_to_open = 0;
	static const int file_success = 1;
};


//������ ����
class GameField
{
public:
	GameField(GameState& gs)
	{
		this->currentGameState = &gs;
		std::vector<bool> temp;
		//������� ��������� ��������
		_field.reserve(currentGameState->heigthOfField);
		for (size_t y = 0; y < currentGameState->heigthOfField; y++)
		{
			temp.resize(0);
			for (size_t i = 0; i < currentGameState->wigthOfField; i++)
			{
				temp.push_back(0);
			}
			_field.push_back(temp);
		}

		//�������� ������
		for (const Cell& cell : currentGameState->cellsCoordinates)
		{
			if (cell.isAlive) this->_field[cell.x][cell.y] = AliveCell;
		}
	}

	~GameField() = default;		//����������
	//������������� ����������, ���� ��� ����� �� ������� ����
	int mapXCoordinate(int x)
	{
		if (x >= currentGameState->wigthOfField) return x % currentGameState->wigthOfField;
		if (x < 0) return currentGameState->wigthOfField - 1;
		return x;
	}

	int mapYCoordinate(int y)
	{
		if (y >= currentGameState->heigthOfField) return y % currentGameState->heigthOfField;
		if (y < 0) return currentGameState->heigthOfField - 1;
		return y;
	}
	//��������� ����, ������� �������, ���-�� ���������, ���-�� �������
	void updateField()
	{
		std::vector<Cell> cells;
		int neighbours, _x_minus, _x, _x_plus, _y, _y_minus, _y_plus;

		for (size_t y = 0; y < currentGameState->heigthOfField; y++) {
			for (size_t x = 0; x < currentGameState->wigthOfField; x++) {
				bool isAlive = _field[x][y];

				neighbours = 0;

				for (int i = -1; i < 2; i++) {
					for (int j = -1; j < 2; j++) {
						_x = mapXCoordinate(x + i);
						_y = mapYCoordinate(y + j);
						if (x == _x && y == _y) continue;
						if (_field[_x][_y] == AliveCell) neighbours++;
					}
				}

				if (shouldBeBorn(neighbours) && !isAlive) cells.push_back(Cell(x, y));
				if (shouldSurvive(neighbours) && isAlive) cells.push_back(Cell(x, y));
				if (!shouldSurvive(neighbours) && isAlive) cells.push_back(Cell(x, y, false));
			}
		}
		//��������\�������
		for (auto& cell : cells)
		{
			if (cell.isAlive) this->_field[cell.x][cell.y] = AliveCell;
			else this->_field[cell.x][cell.y] = EmptyOrDead;
		}
	}

	//������� ������� � ����
	void showField(int numOfIteration = 1)
	{
		
		for (size_t i = 0; i < numOfIteration; i++)
		{
			std::system("cls");
			this->updateField();
			std::cout << currentGameState->universeName << std::endl;
			std::cout << "B" << currentGameState->birthRules << "/S" << currentGameState->survivalRules << std::endl;

			for (int y = 0; y < currentGameState->heigthOfField; y++)
			{
				for (int x = 0; x < currentGameState->wigthOfField; x++)
				{
					if (this->_field[x][y]) std::cout << &ac << " ";
					else std::cout << " " << " ";
				}
				std::cout << std::endl;
			}

			Sleep(100);
		}
	}

	//��������� ��������� � ����
	void dumpField(std::wstring outputFile)
	{
		std::ofstream fout(outputFile);
		fout << "#N " << currentGameState->universeName << std::endl;
		fout << "#R " << "#B" << currentGameState->birthRules << "/" << "S" << currentGameState->survivalRules << std::endl;
		fout << "#S " << currentGameState->wigthOfField << " " << currentGameState->heigthOfField << std::endl;

		for (size_t i = 0; i < currentGameState->cellsCoordinates.size(); i++)
		{
			if (currentGameState->cellsCoordinates[i].isAlive) 
				fout << currentGameState->cellsCoordinates[i].x << " " << currentGameState->cellsCoordinates[i].y << std::endl;
		}
	}

private:
	GameState* currentGameState;
	std::vector<std::vector<bool>> _field;

	//������� ��������� �� ������������ �������
	bool shouldSurvive(int neighbours)
	{
		size_t found = currentGameState->survivalRules.find(std::to_string(neighbours));
		if (found == std::string::npos) return false;
		else return true;
	}
	//������� �������� �� ������������ �������
	bool shouldBeBorn(int neighbours)
	{
		size_t found = currentGameState->birthRules.find(std::to_string(neighbours));
		if (found == std::string::npos) return false;
		else return true;
	}
};

//����� ������
class CommandState {
public:
	CommandState(GameState& gs) { currentGameState = &gs; }
	~CommandState() = default;

	int readCommand()
	{
		std::cin >> _currentCommand;

		if (_currentCommand == "exit") return this->_exit;

		if (_currentCommand == "dump") return this->_dump;

		if (_currentCommand == "tick") return this->_tick;

		if (_currentCommand == "help") return this->_help;

		return -1;

	}

	const static int _exit = 0;
	const static int _dump = 1;
	const static int _tick = 2;
	const static int _help = 3;

private:
	GameState* currentGameState;
	std::string _currentCommand;
};


int main(int argc, char** argv)
{
	setlocale(LC_ALL, "Russian");			//��� ������� ������
	int commandArg = 0, iterations = -1, gameMode = OnlineGameMode;	//�������, ���-�� ��������, ������\�������
	std::wstring inputFile, outputFile;		//������� � �������� ����
	//���� ��������� ����� ���, �� 0 
	std::string _temp;
	if (argc == 1) _temp = "";
	else _temp = argv[1];
	inputFile = std::wstring(_temp.begin(), _temp.end());

	//��� ������� ������
	if (argc > 1)
	{
		gameMode = OfflineGameMode;
		//���������: ���������� �������� � ���� ������
		for (int i = 1; i < argc; i++)
		{
			if (argv[i] == std::string("-i"))
			{
				iterations = std::stoi(argv[i + 1]);
			}

			if (argv[i] == std::string("-o"))
			{
				std::string temp = argv[i + 1];
				outputFile = std::wstring(temp.begin(), temp.end());
			}
		}
	}

	Messages msg;
	GameState* gs = new GameLoader(inputFile);
	GameField game(*gs);
	CommandState command(*gs);
	std::wstring commandString;

	//��� ��������
	if (gameMode == OfflineGameMode)
	{
		game.showField(iterations);
		game.dumpField(outputFile);
		std::wcout << "Saved in " << outputFile << std::endl;
		exit(0);
	}

	msg.showReference();
	//��������� ����� (������)
	std::cout << std::endl << "Enter tick <number of iteration> to start the game!" << std::endl;
	std::cout << "������� tick <���������� ��������> ����� ������ ����!" << std::endl;


	//������
	while (1)
	{
		commandArg = command.readCommand();

		if (CommandState::_exit == commandArg) {
			delete gs;
			exit(0);
		}

		else if (CommandState::_dump == commandArg) {
			std::wcin >> commandString;
			game.dumpField(commandString);
			msg.done();
		}

		else if (CommandState::_tick == commandArg) {
			std::cin >> iterations;
			game.showField(iterations);
		}

		else if (CommandState::_help == commandArg) {
			msg.showReference();
		}

		else {
			msg.errUnknownCommand();
		}
	}

	delete gs;
	return 0;
}