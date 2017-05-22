#include <iostream>
#include <algorithm>
#include <vector>
#include <set>
#include <string>
#include <fstream>
#include <sstream>
#include <cctype>
#include <ctime>
#include <thread> 
#include <mutex>
#include <windows.h>
 
using namespace std;
 
std::mutex mtx; 

vector<string>* CreateText(string fileName)
{
	vector<string>* vec_text = new vector<string>;
	string in_word;

	ifstream file(fileName);	
	if (!file.is_open())
	{
		cout << "File not open" << endl;
		return nullptr;
	}

	file.seekg(0, ios::end); 
	int length = file.tellg(); 
	if ( length == 0 )
	{
		cout << "File is empty" << endl;
		return nullptr;
	}
	file.seekg(0, ios::beg); 


	while (file >> in_word)
	{
		string out_word = "";

		for (char& symbol : in_word)
		{
			if (!iswalpha((unsigned char)symbol)) 
			{
				continue;
			}
			symbol = tolower(symbol);
			out_word.push_back(symbol);
		}

		vec_text->push_back(out_word);
	}

	return vec_text;
}

void ParallelCreateGramms(vector<string>* vec_text, int gramma_size, int begin, int end, vector<vector<string> >* gramma)
{
	if( !vec_text->size() || gramma_size < 1)
	{
		cout << "Input text vector or size of gramma is null" << endl;
		return;
	}

	for (int i = begin; i < end; i++)
	{
		vector<string> vec_temp;

		for (int j = i; j < i + gramma_size; j++)
		{		
			vec_temp.push_back(vec_text->at(j));			
		}

		mtx.lock();
		gramma->push_back(vec_temp);
		mtx.unlock();		
	}

	return;
}

void ParallelCompare(vector<string>* vec_gramma, vector<string>* vec_text, int &result)
{
	if (vec_text->size() < vec_gramma->size())
	{
		cout << "Text size is smaller than gramma" << endl;
		return;
	}

	int compare_number = 0;

	for (int i = vec_gramma->size() - 1; i < vec_text->size(); i++)
	{
		bool equ = true;
		for (int j = 0; j <= vec_gramma->size() - 1; j++)
		{
			string string_text, string_gramma;
			string_text = vec_text->at(i - j);
			string_gramma = vec_gramma->at(vec_gramma->size() - 1 - j);

			if (string_text != string_gramma) 
			{
				equ = false;
				break;
			}
		}

		if (equ == true)
		{
			compare_number++;
		}
	}
	
	result = compare_number;
}

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "rus");

//-------------------------Создание текста и 3-грамм--------------------
	unsigned int start_clock = clock();

	vector<string> *Text = CreateText("data.txt");
	if (Text == nullptr)
	{
		delete Text;
        return 1;
	}

	vector<vector<string> > *Gramms = new vector<vector<string> >;
	vector<thread*> Threads;
	int NumberOfThreads = 4;
	int step = (Text->size()-2) / NumberOfThreads;
	int begin = 0;
	int end = step;

	for (int s = 1; s <= NumberOfThreads; s++)
		{
			auto th = new std::thread(ParallelCreateGramms, Text, 3, begin, end, std::ref(Gramms));
			Threads.push_back(th);

			begin = end;

			if (s == NumberOfThreads-1)
			{
				end = Text->size() - 2;
			}
			else
			{
				end = end + step;
			}		
		}

	for (auto &th : Threads)
		{
			th->join();
			delete th;
		}

	Threads.clear();

	sort (Gramms->begin(), Gramms->end());
	vector<vector<string> >::iterator it;
	it = unique (Gramms->begin(), Gramms->end()); 
	Gramms->resize(distance(Gramms->begin(),it));

//-----------------Подсчет количества повторений 3-грамм----------------
	unsigned int middle_clock = clock();

	int n = 0;
	begin = 0;						
	end = NumberOfThreads;	
	int loop = Gramms->size() /  NumberOfThreads;
	step = Gramms->size() / loop;
	vector<int> Compares(Gramms->size());

	for (int l = 1; l <= loop; l++)
		{
		for (int g = begin; g < end; g++)
			{
				auto th = new std::thread(ParallelCompare, &Gramms->at(g), Text, std::ref(Compares.at(g)));
				Threads.push_back(th);
			}
			begin = end;

			if (l == loop-1) 
				end = Gramms->size();
			else
				end = end + step;

		for (auto &th : Threads)
			{
				th->join();
				delete th;
			}
			Threads.clear();
		}

//-----------------Запись результатов в файл----------------
	ofstream fout("result.txt");
	if (!fout.is_open())
	{
		cout << "Файл не может быть открыт!\n";
	}

	for (int g = 0; g < Gramms->size(); g++)
	{
		int all_gramms_number = Gramms->size();
		float prob = ((float)Compares.at(g) / (float)all_gramms_number) * 100;

		fout << "Gramma: " << endl << endl;
		for(int c=0; c<3; c++)
		{
			fout << Gramms->at(g).at(c) << endl;
		}

		fout << endl;
		fout << "All gramms number: " << all_gramms_number << endl;
		fout << "Compare number: " << Compares.at(g) << endl;
		fout << "Probably: " << prob << "%" << endl;
		fout << "--------------------------------" << endl;
	}

	fout.close();
	cout<< "Ready!" <<endl;

	delete Text;
	delete Gramms;

	unsigned int end_clock = clock();
	unsigned int all_time = (end_clock - start_clock);
	unsigned int compare_time = (end_clock - middle_clock);
	unsigned int create_time = (middle_clock - start_clock);
	cout << "Общее время выполнения: " << all_time << endl;   
	cout << "Время выполнения создания текста и 3-грамм: " << create_time <<endl;
	cout << "Время выполнения подсчета количества 3-грамм: " << compare_time << endl; 

	return 0;
}