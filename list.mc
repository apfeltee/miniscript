
function oprinter(str)
{
    print(str)
}

function Node(element)
{
    // constructor
    let self = {}
    self.element = element;
    self.next = null
    return self
}

// linkedlist class
function LinkedList()
{
    let self = {}
    self.head = null;
    self.size = 0;
    // adds an element at the end
    // of list
    self.add = function(element)
    {
        // creates a new node
        let node = Node(element);
        // to store current node
        let current;
        // if list is Empty add the
        // element and make it head
        if (self.head == null)
        {
            self.head = node;
        }
        else
        {
            current = self.head;
            // iterate to the end of the
            // list
            while (current.next)
            {
                current = current.next;
            }
            // add node
            current.next = node;
        }
        self.size++;
    }

    // insert element at the position index
    // of the list
    self.insertAt = function(element, index)
    {
        if (index < 0 || index > self.size)
        {
            return println("Please enter a valid index.");
        }
        else
        {
            // creates a new node
            let node = Node(element);
            let curr;
            let prev;
            curr = self.head;
            // add the element to the
            // first index
            if (index == 0)
            {
                node.next = self.head;
                self.head = node;
            }
            else
            {
                curr = self.head;
                let it = 0;
                // iterate over the list to find
                // the position to insert
                while (it < index)
                {
                    it++;
                    prev = curr;
                    curr = curr.next;
                }
                // adding an element
                node.next = curr;
                prev.next = node;
            }
            self.size++;
        }
    }

    // removes an element from the
    // specified location
    self.removeFrom = function(index)
    {
        if (index < 0 || index >= self.size)
        {
            println("Please Enter a valid index");
            return;
        }
        else
        {
            let curr = 0
            let prev = 0
            let it = 0;
            curr = self.head;
            prev = curr;
            // deleting first element
            if (index == 0)
            {
                self.head = curr.next;
            }
            else
            {
                // iterate over the list to the
                // position to remove an element
                while (it < index)
                {
                    it++;
                    prev = curr;
                    curr = curr.next;
                }
                // remove the element
                prev.next = curr.next;
            }
            self.size--;
            // return the remove element
            return curr.element;
        }
    }

    // removes a given element from the
    // list
    self.removeElement = function(element)
    {
        let current = self.head;
        let prev = null;
        // iterate over the list
        while (current != null)
        {
            // comparing element with current
            // element if found then remove the
            // and return true
            if (current.element == element)
            {
                if (prev == null)
                {
                    self.head = current.next;
                }
                else
                {
                    prev.next = current.next;
                }
                self.size--;
                return current.element;
            }
            prev = current;
            current = current.next;
        }
        return -1;
    }


    // finds the index of element
    self.indexOf = function(element)
    {
        let count = 0;
        let current = self.head;
        // iterate over the list
        while (current != null)
        {
            // compare each element of the list
            // with given element
            if (current.element == element)
            {
                return count;
            }
            count++;
            current = current.next;
        }
        // not found
        return -1;
    }

    // checks the list for empty
    self.isEmpty = function()
    {
        return self.size == 0;
    }

    // gives the size of the list
    self.size_of_list = function()
    {
        return self.size;
    }

    // prints the list items
    self.printList = function()
    {
        let curr = self.head;
        let str = "";
        oprinter("");
        while (curr)
        {
            oprinter(curr.element)
            oprinter(" ");
            curr = curr.next;
        }
        oprinter("\n")
    }
    return self
}

// creating an object for the
// Linkedlist class
let ll = LinkedList();

// testing isEmpty on an empty list
// returns true
println(ll.isEmpty());

// adding element to the list
ll.add(10);

// prints 10
ll.printList();

// returns 1
println(ll.size_of_list());

// adding more elements to the list
ll.add(20);
ll.add(30);
ll.add(40);
ll.add(50);

// returns 10 20 30 40 50
ll.printList();

// prints 50 from the list
println("is element removed? ", ll.removeElement(50));

// prints 10 20 30 40
ll.printList();

// returns 3
println("Index of 40 ", ll.indexOf(40));

// insert 60 at second position
// ll contains 10 20 60 30 40
ll.insertAt(60, 2);

ll.printList();

// returns false
println("is List Empty? ", ll.isEmpty());

// remove 3rd element from the list
println(ll.removeFrom(3));

// prints 10 20 60 40
ll.printList();