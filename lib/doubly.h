template<class T>
struct Node
{
    T key;
    Node<T>* next;
    Node<T>* prev;

    Node(const T& key)
    {
        this->key = key;
        this->next = nullptr;
        this->prev = nullptr;
    }
};

namespace std
{
template<class T>
class vector
{
public:
    vector()
    {
        count = 0;
        head = nullptr;
    }

    void push_back(T data)
    {
        count++;
        Node<T>* N = new Node<T>(data);
        Node<T>* pt = new Node<T>(data);
        N->next = nullptr;
        N->prev = nullptr;
        pt = head;
        if (head == nullptr)
        {
            head = N;
            return;
        }

        while (pt->next)
            pt = pt->next;
        
        pt->next = N;
        N->prev = pt;
        N->next = nullptr;
    }

    T& operator[](size_t index)
    {
        Node<T>* ret = head;
        for (size_t i = 0; i < index; i++)
        {
            if (ret == nullptr)
                break;
            
            ret = ret->next;
        }

        return ret->key;
    }

    size_t size()
    {
        return count;
    }
private:
    Node<T>* head;
    int count;
};
}