// Huffman.cpp : Этот файл содержит функцию "main". Здесь начинается и заканчивается выполнение программы.
//

#include <iostream>
#include <fstream>
#include <list>
using namespace std;
struct Symbol//символ
{
	char sym;//значение
	int count,bit_count;//частота и число бит в коде
	long long code;//код
	int id;
	string str_code;
	Symbol()
	{

	}
	Symbol(char sym, int count, int id)
	{
		this->sym = sym; this->count = count; this->id = id;
	}
	Symbol(char sym,int bit_count, long long code)
	{
		this->sym = sym; this->code = code; this->bit_count = bit_count;
	}
	bool operator<(Symbol sym)
	{
		return count < sym.count;
	}
	bool operator>(Symbol sym)
	{
		return count > sym.count;
	}
	friend bool operator >(const Symbol& a, const  Symbol& b);
	void WriteCompressedBits(char*& arr, int &free_bits, int &number_current_byte)//запись кода в массив байт
	{
		long long copy_code = code;
		int tail_with_torso_bit = (bit_count - free_bits); tail_with_torso_bit*=(tail_with_torso_bit > 0);//число бит хвостовой и туловищной части
		int torso_bytes= tail_with_torso_bit/8, tail_bit_count= tail_with_torso_bit %8;//число туловищных байт и хвостовых бит
		
		int tail_byte_number = number_current_byte + torso_bytes + (tail_bit_count > 0);
		arr[tail_byte_number]|= (copy_code << (8 - tail_bit_count)) & 0xFF;//копирование хвостовых бит
		copy_code >>= ((8-tail_bit_count)%8);
		for (int i = torso_bytes; i > 0; i--)//копирование туловищных бит
		{
			arr[number_current_byte + i] = copy_code & 0xFF;
			copy_code >>= 8;
		}
		int head_bit_count = bit_count - tail_with_torso_bit;
		arr[number_current_byte]|= (copy_code<<(free_bits- head_bit_count%8)) & 0xFF;
		number_current_byte = tail_byte_number + (bit_count >= free_bits && tail_with_torso_bit==0);
		free_bits = 8 - tail_bit_count- bit_count*(bit_count<free_bits);
	}
	void Save(ofstream& f)
	{
		f.write(&sym, 1);//запоминание символа
		f.write((char*)&bit_count, 4);//запоминание длины кода
		f.write((char*)&code, sizeof(long long));//запоминание кода
	}
	void ShowCode()
	{
		cout << sym << " ";
		int copy_code = code;
		for (int i = bit_count-1; i>=0; i--)
		{
			cout << ((copy_code>>i) & 0x1);
		}
		cout << endl;
	}
};
bool operator >(const Symbol& a, const  Symbol& b)//Перегрузка >
{
	return a.count > b.count;
}


class TreeNode//узел дерева, абстрактный
{

protected:
	int count;//частота
public:
	TreeNode(){}
	TreeNode(int count) { this->count = count; }
	int Count() { return count; }
	virtual void BuildCode(long long digit_code,int bit_count) = 0;//рекурсивное построение кода
};

class SymbolTreeNode :public TreeNode//листьевая вершина дерева
{
	Symbol *symbol;
public:
	SymbolTreeNode(Symbol *symbol, int count) :TreeNode(count)
	{ this->symbol = symbol;}
	virtual void BuildCode(long long digit_code, int bit_count)//рекурсивное построение кода
	{
		symbol->bit_count = (bit_count>0)*bit_count;
		symbol->code = digit_code;
	}
};

class ParentTreeNode:public TreeNode//предки листьевых деревье
{
protected:
	TreeNode* left, *right;//ссылки на левые и правые дочерние вершины

public:
	ParentTreeNode(TreeNode* left, TreeNode* right,int count):TreeNode(count)
	{
		this->left = left; this->right = right; 
	}
	virtual void BuildCode(long long digit_code, int bit_count)//рекурсивное построение кода
	{
		left->BuildCode(digit_code<<1, bit_count+1);
		right->BuildCode((digit_code << 1)& 0x1, bit_count + 1);
	}
};

class HuffmamTree//дерево хаффмана
{
	list<Symbol> *list_symbol;//список символов
	TreeNode* root;//корень дерева
	Symbol* HashSymbol[256];//массив
public:
	HuffmamTree() {}
	HuffmamTree(list<Symbol>* list_symbol)
	{
		this->list_symbol = list_symbol;
		//BuildTree();
	}
	void BuildTree()
	{
		list<TreeNode*> list_nodes;
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			Symbol* sym = &*iter;
			HashSymbol[sym->sym + 127] = sym;
			list_nodes.push_back(new SymbolTreeNode(sym, iter->count));//инициализация листьев дерева
		}
		int count = list_nodes.size();
		root = *list_nodes.begin();
		//list_nodes.sort(/*greater<TreeNode>()*/);
		for (int i = 0; i < count - 1; i++)//построение вершин дерева
		{
			list<TreeNode*>::iterator min_iter_1, min_iter_2;
			FindMinPair(list_nodes, min_iter_1, min_iter_2);//поиск минимальных вершин без родителей
			TreeNode* min_node1 = *min_iter_1, * min_node2 = *min_iter_2;
			int sum = min_node1->Count()+ min_node2->Count();
			cout << min_node1->Count() << " " << min_node2->Count() << endl;
			list_nodes.erase(min_iter_1); list_nodes.erase(min_iter_2);//удаление двух вершин из списка
			root = new ParentTreeNode(min_node2, min_node1, sum);//новая безродная вершина дерева в списке
			list_nodes.push_back(root);
		}
		BuildCode();//построение оптимальног кода
	}
	void FindMinPair(list<TreeNode*> &list_nodes,list<TreeNode*>::iterator& min_iter_1, list<TreeNode*>::iterator& min_iter_2)//поиск пар минимальных безродных вершин
	{
		cout << "---------------------\n";
		auto first = list_nodes.begin(), second= ++list_nodes.begin();
		TreeNode* first_node = *first, *second_node=*second;
		if (first_node->Count() <= second_node->Count())
		{
			min_iter_1 = first;
			min_iter_2 = second;
		}
		else
		{
			min_iter_2 = first;
			min_iter_1 = second;
		}
		for (auto iter = ++second; iter != list_nodes.cend(); iter++)
		{
			TreeNode* node = *iter, * min_node1 = *min_iter_1, * min_node2 = *min_iter_2;
			cout << node->Count() << endl;
			if (node->Count() <= min_node1->Count())
			{
				if (node->Count() == min_node1->Count() && min_node1->Count()<= min_node2->Count())
				{
					min_iter_2 = min_iter_1;
				}
				min_iter_1 = iter;
				
			}
			if (node->Count() < min_node2->Count() && node->Count()> min_node1->Count())
				min_iter_2 =iter;
		}
	}
	void BuildCode()
	{
		root->BuildCode(0, 0);
	}
	int CompressedBitSize()
	{
		int bit_count = 0;
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)
		{
			bit_count += iter->bit_count * iter->count;
		}
		return bit_count;
	}
	char* CreateCompressedBitArray(char* file_bit,long size, long &compess_size)
	{
		int bit_count = CompressedBitSize();
		int tail_bit_count = bit_count % 8;
		int byte_count = bit_count / 8+(tail_bit_count>0);
		char* compressed_array = new char[byte_count];
		memset(compressed_array, 0, byte_count);
		int current_byte = 0, free_bits=8;
		for (int i = 0; i < size; i++)
		{
			int id = file_bit[i] + 127;
			HashSymbol[id]->WriteCompressedBits(compressed_array, free_bits, current_byte);
		}
		compess_size = byte_count;
		return compressed_array;
	}
	void CompressedFile(string filename, char* file_bit, long size)
	{
		ofstream fout;
		fout.open(filename, ios::binary | ios::out);
		fout.write((char*)&size,sizeof(long));//запоминанение размера файла
		int count = list_symbol->size();
		fout.write((char*)&count, sizeof(int));//запоминание числа различных символов
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)//запоминание ключевой информации
		{
			Symbol* sym = &*iter;
			sym->Save(fout);
		}
		long size_comressed = 0;
		char* compressed = CreateCompressedBitArray(file_bit, size, size_comressed);
		fout.write(compressed, size_comressed);
		fout.close();
	}
};

struct PairKeySymbol
{
	int len;
	long long code;
public:
	PairKeySymbol(long long code, int len) :len(len), code(code) {}
};

class Board
{
	HuffmamTree tree;
	char* FileByte;
	long file_size;
public:
	Board() {}
	void ReadFile(string filename)
	{
		list<Symbol> *list_symbol=new list<Symbol>();
		ifstream ifs(filename, ios::binary | ios::ate);
		ifstream::pos_type pos = ifs.tellg();
		file_size = pos;
		FileByte = new char[file_size];//массив байт файла
		//MessageBox::Show(length + "");
		ifs.seekg(0, ios::beg);
		ifs.read(FileByte, file_size);//чтение в массив байт
		int* Alphabet = new int[256];//частоты всех возможных символов
		memset(Alphabet, 0, sizeof(int) * 256);//обнуление массива
		for (int i = 0; i < file_size; i++)//подсчёт частот символов
			Alphabet[127 + FileByte[i]]++;
		int id = -1;
		for (int i = 0; i < 256; i++)//инициализация всех символов файла
		{
			int count = Alphabet[i];
			if (count > 0)
			{
				id++;
				list_symbol->push_back(Symbol(i - 127, count, id));
			}
		}
		ifs.close();
		tree= HuffmamTree(list_symbol);
		tree.BuildTree();
	}
	void CompressFile(string filename)
	{
		ReadFile(filename);
		tree.CompressedFile(filename + ".huf", FileByte, file_size);
	}
	void ReadCompressedFile(string filename)
	{
		list<Symbol>* list_symbol = new list<Symbol>();
		ifstream ifs(filename, ios::binary | ios::ate);
		ifs.read((char*)&file_size, sizeof(long));//чтение исходного размера
		FileByte = new char[file_size];//массив байт файла
		//MessageBox::Show(length + "");
		int symbols_count = 0;
		ifs.read((char*)&symbols_count, sizeof(int));//чтение числа символов
		for (int i = 0; i < symbols_count; i++)//чтение ключевой информации
		{
			long long code; char s; int len;
			ifs.read(&s, 1);
			ifs.read((char*)&len, 4);
			ifs.read((char*)&code, sizeof(long long));
			list_symbol->push_back(Symbol(s,len, code));
		}
		ifs.read(FileByte, file_size);//чтение в массив байт

		ifs.close();
		tree = HuffmamTree(list_symbol);
		for (auto iter = list_symbol->begin(); iter != list_symbol->cend(); iter++)//запоминание ключевой информации
		{
			iter->ShowCode();
		}
	}
};



int main()
{
	setlocale(LC_ALL, "Russian");
	Board board;
	board.CompressFile("input.txt");
	board.ReadCompressedFile("input.txt.huf");
	system("pause");
}

// Запуск программы: CTRL+F5 или меню "Отладка" > "Запуск без отладки"
// Отладка программы: F5 или меню "Отладка" > "Запустить отладку"

// Советы по началу работы 
//   1. В окне обозревателя решений можно добавлять файлы и управлять ими.
//   2. В окне Team Explorer можно подключиться к системе управления версиями.
//   3. В окне "Выходные данные" можно просматривать выходные данные сборки и другие сообщения.
//   4. В окне "Список ошибок" можно просматривать ошибки.
//   5. Последовательно выберите пункты меню "Проект" > "Добавить новый элемент", чтобы создать файлы кода, или "Проект" > "Добавить существующий элемент", чтобы добавить в проект существующие файлы кода.
//   6. Чтобы снова открыть этот проект позже, выберите пункты меню "Файл" > "Открыть" > "Проект" и выберите SLN-файл.
