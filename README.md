Assignment 3: Python GUI and Databases

Run the command 'make parser' from the assign3 folder to create the shared library
Run the GUI from the bin folder by typing 'python3 A3main.py' into the command line

1. Description
A GUI that displays a list of valid vcf files, and allows the user to edit them or create new ones, and adds to/updates a database as changes are made. The user can also view two database queries.

2. Checklist

DB Login view:
• Prompts the user to enter the database name, username and password
• Creates a connection to the database if entered credentials are correct
• Displays an error popup if a DB connection could not be established
• Creates tables in the DB if they do not already exist

Main view:
• Displays all the filenames in the cards folder
• Displays Create, Edit, DB Queries and Exit buttons
• Does not display invalid files
• Files are clickable
• Correctly updates when a file is edited or a new file is created
• Tables in the DB are correctly populated with the file information
• Exists program

Card Detail view:
• Displays the correct information for the selected file (filename, contact name, birthday, anniversary, number of optional properties)
• Information in the contact field can be modified
• Changes to the contents of the Card Detail view are reflected on the disk and in the DB tables
• Correctly exists to main view

Create vCard:
• Creates a new Card object, passes it to the parser, saves it to the vCard file
• User can enter filename and contact name
• Validates user input and does not allow a user to create an invalid vCard file
• Files updated in the DB

DB View:
• Runs the two required queries and correctly displays the information stored in the tables
• Correctly exists to main view
