function randName() {
    var myArray = ['James', 'Simon', 'Alana', 'William', 'Sacha', 'Nicole', 'Sandra', 'Jamie', 'Jayden', 'Tayla', 'Tia', 'Kyle'];
    var rand = myArray[Math.floor(Math.random() * myArray.length)];
    return rand;
}

