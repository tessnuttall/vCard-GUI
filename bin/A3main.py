#!/usr/bin/env python3

from asciimatics.widgets import Frame, ListBox, Layout, Divider, Text, \
    Button, TextBox, Widget
from asciimatics.scene import Scene
from asciimatics.screen import Screen
from asciimatics.exceptions import ResizeScreenError, NextScene, StopApplication
import sys
import sqlite3
import os
import ctypes
import mysql.connector
import datetime
import time
from asciimatics.widgets import PopUpDialog

# Loads the shared library
lib = ctypes.CDLL("./libvcparser.so")

# Defines the Nodes of the list
class Node(ctypes.Structure):
    pass

Node._fields_ = [
    ("data", ctypes.c_void_p),
    ("previous", ctypes.POINTER(Node)),
    ("next", ctypes.POINTER(Node))
]

# Defines the List
class List(ctypes.Structure):
    _fields_ = [
        ("head", ctypes.POINTER(Node)),
        ("tail", ctypes.POINTER(Node)),
        ("length", ctypes.c_int),
        ("deleteData", ctypes.CFUNCTYPE(None, ctypes.c_void_p)),
        ("compare", ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p)),
        ("printData", ctypes.CFUNCTYPE(ctypes.c_char_p, ctypes.c_void_p))
    ]

# Defines the DateTime struct
class DateTime(ctypes.Structure):
    _fields_ = [
        ("UTC", ctypes.c_bool),
        ("isText", ctypes.c_bool),
        ("date", ctypes.c_char_p),
        ("time", ctypes.c_char_p),
        ("text", ctypes.c_char_p)
    ]

    def __init__(self, date="", time="", text="", UTC=False, isText=False):
        super().__init__()
        self.UTC = UTC
        self.isText = isText
        self.date = date.encode('utf-8') if date is not None else ''
        self.time = time.encode('utf-8') if time is not None else ''
        self.text = text.encode('utf-8') if text is not None else ''


# Defines the Parameter struct
class Parameter(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("value", ctypes.c_char_p)
    ]

# Defines the Property struct
class Property(ctypes.Structure):
    _fields_ = [
        ("name", ctypes.c_char_p),
        ("group", ctypes.c_char_p),
        ("parameters", ctypes.POINTER(List)),
        ("values", ctypes.POINTER(List))
    ]

# Defines the Card struct
class Card(ctypes.Structure):
    _fields_ = [
        ("fn", ctypes.POINTER(Property)),
        ("optionalProperties", ctypes.POINTER(List)),
        ("birthday", ctypes.POINTER(DateTime)),
        ("anniversary", ctypes.POINTER(DateTime))
    ]


# Allows the python code to use C functions
lib.createCard.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.POINTER(Card))]
lib.createCard.restype = ctypes.c_int

lib.validateCard.argtypes = [ctypes.POINTER(Card)]
lib.validateCard.restype = ctypes.c_int

lib.deleteCard.argtypes = [ctypes.POINTER(Card)]
lib.deleteCard.restype = None

lib.writeCard.argtypes = [ctypes.c_char_p, ctypes.POINTER(Card)]
lib.writeCard.restype = ctypes.c_int

# Displays the login page
class LoginView(Frame):
    def __init__(self, screen):
        super(LoginView, self).__init__(screen, 
                                        screen.height * 2 // 3, 
                                        screen.width * 2 // 3,
                                        hover_focus=True, 
                                        can_scroll=False, 
                                        title="Login",
                                        reduce_cpu=True)
        
        # Defines the username, password and database name
        self.username = Text("Username:", "username")
        self.password = Text("Password:", "password")
        self.dbname = Text("Database Name:", "database")
        
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        
        # Add widgets (username, password, dbname) to the layout
        layout.add_widget(self.username)
        layout.add_widget(self.password)
        layout.add_widget(self.dbname)
        
        # Buttons for OK and Cancel
        layout2 = Layout([1, 1])
        self.add_layout(layout2)
        
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 1)
        
        self.fix()

    # If the user clicks ok
    def _ok(self):
        global _db_connection
        # Gets the users input
        username = self.username.value
        password = self.password.value
        dbname = self.dbname.value
        
        try:
            # Connects to the database
            _db_connection = mysql.connector.connect(
                host="dursley.socs.uoguelph.ca",
                database=dbname,
                user=username,
                password=password
            )
            _db_connection.autocommit = True

            # If connection is successful, move onto ListView
            raise NextScene("Main")
        
        # If connection is not successful shows a popup error box
        except mysql.connector.Error:
            self.scene.add_effect(PopUpDialog(self.screen, "Invalid credentials.", ["OK"]))
    
    # If the user clicks cancel
    def _cancel(self):
        # Closes the application
        raise StopApplication("Login canceled.")

# A class for the contact database
class ContactModel():
    def __init__(self):
        global _db_connection
        self.cursor = None

        # Current contact when editing
        self.current_id = None
        self.current_card = None

        if _db_connection is not None:
            self.create_tables()

    def create_tables(self):

        # Creates the FILE table
        self.cursor.execute('''
            CREATE TABLE IF NOT EXISTS FILE(
                file_id INT AUTO_INCREMENT PRIMARY KEY,
                file_name VARCHAR(60) NOT NULL,
                last_modified DATETIME,
                creation_time DATETIME NOT NULL
            )
        ''')

        # Creates the CONTACT table
        self.cursor.execute('''
            CREATE TABLE IF NOT EXISTS CONTACT(
                contact_id INT AUTO_INCREMENT PRIMARY KEY,
                name VARCHAR(256) NOT NULL,
                birthday DATETIME DEFAULT NULL,
                anniversary DATETIME DEFAULT NULL,
                file_id INT NOT NULL,
                FOREIGN KEY (file_id) REFERENCES FILE(file_id) ON DELETE CASCADE
            )
        ''')

    def get_all_contacts(self):
        # Gets information from database tables
        query = '''
            SELECT c.contact_id, c.name, c.birthday, c.anniversary, f.file_name
            FROM CONTACT c
            INNER JOIN FILE f ON c.file_id = f.file_id
            ORDER BY c.name
        '''
        self.cursor.execute(query)
        result = self.cursor.fetchall()

        # Formats the information
        contacts = []
        for row in result:
            contact = {
                "contact_id": row[0],
                "name": row[1],
                "birthday": row[2],
                "anniversary": row[3],
                "file_name": row[4]
            }
            contacts.append(contact)
        
        return contacts

    def find_all_june(self):
        # Gets contacts born in June and sorts them by age
        query = '''
            SELECT c.name, c.birthday, f.last_modified
            FROM CONTACT c
            INNER JOIN FILE f ON c.file_id = f.file_id
            WHERE MONTH(c.birthday) = 6
            ORDER BY DATEDIFF(f.last_modified, c.birthday) DESC
        '''
        self.cursor.execute(query)
        result = self.cursor.fetchall()

        # Formats the information
        contacts = []
        for row in result:
            contact = {
                "name": row[0],
                "birthday": row[1]
            }
            contacts.append(contact)
    
        return contacts

    # A method to add a new contact
    def add(self, contact):

        #Extracts the filename and new contact information
        filename = contact.get("filename", "")
        contact_value = contact.get("contact", "")
        filepath = f"./cards/{filename}".encode('utf-8')

        # Check if the filename already exists in the FILE table
        self.cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (filename,))
        existing_file = self.cursor.fetchall()

        # Exits if the file name already exists
        if existing_file:
            return

        # Writes contact to a temporary vcf file
        temp_vcf = "temp.vcf"

        with open(temp_vcf, "w") as file:
            file.write("BEGIN:VCARD\r\n")
            file.write("VERSION:4.0\r\n")
            file.write(f"FN:{contact_value}\r\n")
            file.write("END:VCARD\r\n")

        # Calls createCard and returns an error code
        card_ptr = ctypes.POINTER(Card)()
        error_code = lib.createCard(temp_vcf.encode('utf-8'), ctypes.byref(card_ptr))

        if error_code == 0 and card_ptr:
            # Calls the validateCard function
            validate_result = lib.validateCard(card_ptr)

            if validate_result == 0:
                # Calls writeCard to write the new vcf file
                write_result = lib.writeCard(filepath, card_ptr)

                if write_result == 0:
                    # Gets the time the file was last modified at
                    last_modified = datetime.datetime.fromtimestamp(os.path.getmtime(f"./cards/{filename}"))
                    creation_time = datetime.datetime.now()

                    # Insert into FILE table
                    self.cursor.execute(
                        "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (%s, %s, %s)",
                        (filename, last_modified, creation_time)
                    )
                    _db_connection.commit()

                    # Retrieve the newly inserted file_id
                    self.cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (filename,))
                    file_id_row = self.cursor.fetchall()

                    if file_id_row:
                        file_id = file_id_row[0]
                    else:
                        return

                    # Insert into CONTACT table
                    self.cursor.execute(
                        "INSERT INTO CONTACT (name, file_id) VALUES (%s, %s)",
                        (contact_value, file_id[0])
                    )
                    _db_connection.commit()

    # A method to retrieve a list of contacts
    def get_summary(self):
        if _db_connection is None:
            return []
        else:
            valid_files = []

            # Defines the cursor
            self.cursor = _db_connection.cursor()

            # Retrieves file names from database
            self.cursor.execute("SELECT file_name FROM FILE")
            files_in_db = self.cursor.fetchall()
            files_in_db = {row[0] for row in files_in_db}

            # Loops through the files in the cards folder
            for filename in os.listdir("./cards"):

                if filename.endswith(".vcf"):
                    # Converts the filepath into a string
                    filepath = f"./cards/{filename}".encode('utf-8')

                    # Creates a pointer for the Card to be populated
                    card_ptr = ctypes.POINTER(Card)()

                    # Calls createCard to populate the card object
                    result = lib.createCard(filepath, ctypes.byref(card_ptr))

                    # If the return value is correct
                    if result == 0 and card_ptr:
                        # Calls the validateCard function
                        validate_result = lib.validateCard(card_ptr)

                        # If card is valid
                        if validate_result == 0:
                            # Returns the valid files
                            valid_files.append((filename, filename))

                            # Extracts contact name from the struct
                            if card_ptr.contents.fn and card_ptr.contents.fn.contents.values:
                                first_value = card_ptr.contents.fn.contents.values.contents.head

                                # Stores the contact name
                                if first_value and first_value.contents.data:
                                    contact_name = ctypes.cast(first_value.contents.data, ctypes.c_char_p).value.decode('utf-8')
                                else:
                                    contact_name = None
                            else:
                                contact_name = None

                            # Extracts birthday date and time from struct
                            birthday_date = card_ptr.contents.birthday.contents.date.decode('utf-8') if card_ptr.contents.birthday else None
                            birthday_time = card_ptr.contents.birthday.contents.time.decode('utf-8') if card_ptr.contents.birthday and card_ptr.contents.birthday.contents.time else None

                            # Extracts anniversary date and time from struct
                            anniversary_date = card_ptr.contents.anniversary.contents.date.decode('utf-8') if card_ptr.contents.anniversary else None
                            anniversary_time = card_ptr.contents.anniversary.contents.time.decode('utf-8') if card_ptr.contents.anniversary and card_ptr.contents.anniversary.contents.time else None

                            # Converts the birthday date and time to MySQL format
                            if birthday_date and birthday_time:
                                try:
                                    birthday_str = f"{birthday_date} {birthday_time}"

                                    # Converts the string into MySQL datetime format
                                    format_bday = f"{birthday_str[:4]}-{birthday_str[4:6]}-{birthday_str[6:8]} {birthday_str[9:11]}:{birthday_str[11:13]}:{birthday_str[13:15]}"
                                    birthday_datetime = datetime.datetime.strptime(format_bday, "%Y-%m-%d %H:%M:%S")
                                    birthday = birthday_datetime.strftime("%Y-%m-%d %H:%M:%S")
                                except ValueError:
                                    birthday = None
                            else:
                                birthday = None

                            # Converts the anniversary date and time to MySQL format
                            if anniversary_date and anniversary_time:
                                try:
                                    anniv_str = f"{anniversary_date} {anniversary_time}"

                                    # Converts the string into MySQL datetime format
                                    format_anniv = f"{anniv_str[:4]}-{anniv_str[4:6]}-{anniv_str[6:8]} {anniv_str[9:11]}:{anniv_str[11:13]}:{anniv_str[13:15]}"
                                    anniversary_datetime = datetime.datetime.strptime(format_anniv, "%Y-%m-%d %H:%M:%S")
                                    anniversary = anniversary_datetime.strftime("%Y-%m-%d %H:%M:%S")
                                except ValueError:
                                    anniversary = None
                            else:
                                anniversary = None

                            # Updates last_modified in the database if file already exists
                            if filename in files_in_db:
                                last_modified = datetime.datetime.fromtimestamp(os.path.getmtime(f"./cards/{filename}"))
                                self.cursor.execute(
                                    "UPDATE FILE SET last_modified = %s WHERE file_name = %s",
                                    (last_modified, filename)
                                )
                                _db_connection.commit()

                                # Retrieves the file_id from the FILE table
                                self.cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (filename,))
                                file_id = self.cursor.fetchone()[0]

                                # Checks if that file_id already exists before inserting to table
                                self.cursor.execute("SELECT COUNT(*) FROM CONTACT WHERE file_id = %s", (file_id,))
                                contact_exists = self.cursor.fetchone()[0]

                                # Inserts info into the CONTACT table
                                if contact_exists == 0 and contact_name:
                                    self.cursor.execute(
                                        "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (%s, %s, %s, %s)",
                                        (contact_name, birthday, anniversary, file_id)
                                    )
                                    _db_connection.commit()

                            else:
                                # Adds the file to the database if it is new
                                creation_time = datetime.datetime.now()
                                last_modified = datetime.datetime.fromtimestamp(os.path.getmtime(f"./cards/{filename}"))
                                self.cursor.execute(
                                    "INSERT INTO FILE (file_name, last_modified, creation_time) VALUES (%s, %s, %s)",
                                    (filename, last_modified, creation_time)
                                )
                                _db_connection.commit()

                                # Retrieves the file_id from the FILE table
                                self.cursor.execute("SELECT file_id FROM FILE WHERE file_name = %s", (filename,))
                                file_id = self.cursor.fetchone()[0]

                                # Checks if that file_id already exists before inserting
                                self.cursor.execute("SELECT COUNT(*) FROM CONTACT WHERE file_id = %s", (file_id,))
                                contact_exists = selfcursor.fetchone()[0]

                                # Inserts or updates the CONTACT table
                                if contact_exists == 0 and contact_name:
                                    self.cursor.execute(
                                        "INSERT INTO CONTACT (name, birthday, anniversary, file_id) VALUES (%s, %s, %s, %s)",
                                        (contact_name, birthday, anniversary, file_id)
                                    )
                                    _db_connection.commit()

            # Returns a tuple of valid filenames
            return valid_files

    # A method to retrieve a single contact
    def get_contact(self, contact_id):
        return self._db.cursor().execute(
            "SELECT * from contacts WHERE id=:id", {"id": contact_id}).fetchone()

    # A method to retrieve the currently selected contact
    def get_current_contact(self):
        if self.current_id is None:
            return {"filename": "", "contact": "", "birthday": "", "anniversary": "", "notes": ""}

        # Stores the filename and defines the filepath
        filename = self.current_id
        filepath = f"./cards/{filename}".encode('utf-8')

        # Calls createCard and returns an error code
        card_ptr = ctypes.POINTER(Card)()
        error_code = lib.createCard(filepath, ctypes.byref(card_ptr))

        # If there is an error creating the card
        if error_code != 0 or not card_ptr:
            return {
                "filename": filename,
                "contact": "",
                "birthday": "",
                "anniversary": "",
                "notes": ""
            }

        # Calls cardToString to convert the card to a string
        card_str = ctypes.string_at(lib.cardToString(card_ptr)).decode('utf-8')

        contact, birthday, anniversary = "", "", ""
        total_properties = 0

        # Splits the string at new lines
        for line in card_str.split("\n"):
            # Stores the FN property value
            if line.startswith("FN:"):
                contact = line.split("Value: ", 1)[-1].strip()
            # Stores the entire birthday string
            elif line.startswith("Birthday:"):
                birthday_whole = line.split("Birthday: ", 1)[-1].strip()

                # If birthday is a text value
                if birthday_whole.startswith("circa"):
                    birthday = birthday_whole
                # If the birthday only has a time
                elif birthday_whole.startswith("T"):
                    time_section = birthday_whole[1:]
                    # If time is UTC
                    if time_section.endswith("Z"):
                        time_section = time_section[:-1] + " (UTC)"
                    birthday= f"Time: {time_section}"
                # If both a date and time are in birthday
                elif "T" in birthday_whole:
                    date_section, time_section = birthday_whole.split("T", 1)
                    # If time is UTC
                    if time_section.endswith("Z"):
                        time_section = time_section[:-1] + " (UTC)"
                    birthday = f"Date: {date_section} Time: {time_section}"
                # If there is only a date in birthday
                else:
                    birthday = f"Date: {birthday_whole}"

            # Stores the whole anniversary string
            elif line.startswith("Anniversary:"):
                anniversary_whole = line.split("Anniversary: ", 1)[-1].strip()

                # If the anniversary only has a time
                if anniversary_whole.startswith("T"):
                    time_section = anniversary_whole[1:]
                    # If time is UTC
                    if time_section.endswith("Z"):
                        time_section = time_section[:-1] + " (UTC)"
                    anniversary = f"Time: {time_section}"
                # If anniversary has both a date and time
                elif "T" in anniversary_whole:
                    date_section, time_section = anniversary_whole.split("T", 1)
                    # If time is UTC
                    if time_section.endswith("Z"):
                        time_section = time_section[:-1] + " (UTC)"
                    anniversary = f"Date: {date_section} Time: {time_section}"
                # If anniversary only has a date
                else:
                    anniversary = f"Date: {anniversary_whole}"
            # Counts the number of optional properties
            elif line.startswith("Property:"):
                total_properties += 1

        notes = f"Number of other properties: {total_properties}"

        # Returns strings for displaying
        return {
            "filename": filename,
            "contact": contact,
            "birthday": birthday,
            "anniversary": anniversary,
            "notes": notes
        }

    # A method to update the current contact
    def update_current_contact(self, details):
        #Extracts the filename and new contact information
        filename = details.get("filename", "")
        contact = details.get("contact", "")

        # Ensures the user entered something in the contact field
        if not contact:
            return

        if self.current_id is None:
            self.add(details)
        else:
            # Gets the filepath
            filepath = f"./cards/{filename}".encode('utf-8')
            card_ptr = ctypes.POINTER(Card)()
            error_code = lib.createCard(filepath, ctypes.byref(card_ptr))

        if error_code == 0 and card_ptr:
            # Calls validateCard to ensure changes made are valid
            validate_result = lib.validateCard(card_ptr)

            if validate_result != 0:
                return

            # Updates the FN property value in the card struct
            if card_ptr.contents.fn:
                card_ptr.contents.fn.contents.values.contents.head.contents.data = ctypes.cast(
                    ctypes.create_string_buffer(contact.encode('utf-8')),
                    ctypes.c_void_p
                )

            # Rewrites the updated contact to file
            lib.writeCard(filepath, card_ptr)

            # Updates the database
            self.cursor.execute('SELECT file_id FROM FILE WHERE file_name = %s', (filename,))
            current_file = self.cursor.fetchone()

            if current_file:
                file_id = current_file[0]
                self.cursor.execute('UPDATE CONTACT SET name = %s WHERE file_id = %s', (contact, file_id))
                _db_connection.commit()

    # A method to delete the current contact
    def delete_contact(self, contact_id):
        self._db.cursor().execute('''
            DELETE FROM contacts WHERE id=:id''', {"id": contact_id})
        self._db.commit()

# Displays the list of valid filename on the frame
class ListView(Frame):
    # Takes the screen and contact model as parameters
    def __init__(self, screen, model):
        # Sets the size of the frame, title, etc
        super(ListView, self).__init__(screen,
                                       screen.height * 2 // 3,
                                       screen.width * 2 // 3,
                                       on_load=self._reload_list,
                                       hover_focus=True,
                                       can_scroll=False,
                                       title="vCard List")
        # Save off the model that accesses the contacts database.
        self._model = model

        # Create the form for displaying the list of contacts.
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            # retrieves a list of valid files and their corresponding card objects
            model.get_summary(),

            # Displays the first instances of the valid_files tuple
            name="contacts",
            add_scroll_bar=True,
            on_change=self._on_pick,
            on_select=self._edit)
        
        # Creates buttons
        self._edit_button = Button("Edit", self._edit)
        self._delete_button = Button("DB Queries", self._db_queries)

        # Creates a layout and adds it to the frame
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        # Adds the listbox to the frame
        layout.add_widget(self._list_view)
        layout.add_widget(Divider())

        # Creates a layout
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)

        # Adds buttons to the second layout
        layout2.add_widget(Button("Create", self._add), 0)
        layout2.add_widget(self._edit_button, 1)
        layout2.add_widget(self._delete_button, 2)
        layout2.add_widget(Button("Quit", self._quit), 3)

        self.fix()
        self._on_pick()

    # Gets called when the user selects and item from the contacts list
    def _on_pick(self):
        self._edit_button.disabled = self._list_view.value is None
        self._delete_button.disabled = self._list_view.value is None

    # Updates the list of contacts
    def _reload_list(self, new_value=None):
        self._list_view.options = self._model.get_summary()
        self._list_view.value = new_value

    # Gets called when the user selects the add button
    def _add(self):
        self._model.current_id = None
        raise NextScene("Add Contact")

    # Gets called when the user selects the edit button
    def _edit(self):
        self.save()
        self._model.current_id = self.data["contacts"]
        raise NextScene("Edit Contact")

    def _db_queries(self):
        raise NextScene("DB Queries")

    # Gets called when the user selects the delete button
    def _delete(self):
        self.save()
        self._model.delete_contact(self.data["contacts"])
        self._reload_list()

    @staticmethod
    def _quit():
        raise StopApplication("User pressed quit")

# Displays the frame to edit or view a contact
class ContactView(Frame):
    def __init__(self, screen, model):
        super(ContactView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="vCard Details",
                                          reduce_cpu=True)
        # Save off the model that accesses the contacts database
        self._model = model

        # Create the form for displaying the list of contacts
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)
        layout.add_widget(Text("Filename:", "filename", disabled=True))
        # Allows the user to edit the text in the contacts field
        layout.add_widget(Text("Contact:", "contact",))
        layout.add_widget(Text("Birthday:", "birthday", disabled=True))
        layout.add_widget(Text("Anniversary:", "anniversary", disabled=True))
        layout.add_widget(TextBox(
            Widget.FILL_FRAME, "Other Properties:", "other_props", as_string=True, line_wrap=True, disabled=True))
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 3)
        self.fix()

    def reset(self):
        # Do standard reset to clear out form, then populate with new data
        super(ContactView, self).reset()
        self.data = self._model.get_current_contact()

    def _ok(self):
        self.save()
        self._model.update_current_contact(self.data)
        raise NextScene("Main")

    @staticmethod
    def _cancel():
        raise NextScene("Main")

# Displays the frame to add a new contact
class AddView(Frame):
    def __init__(self, screen, model):
        super(AddView, self).__init__(screen,
                                          screen.height * 2 // 3,
                                          screen.width * 2 // 3,
                                          hover_focus=True,
                                          can_scroll=False,
                                          title="vCard Details",
                                          reduce_cpu=True)
        # Save off the model that accesses the contacts database.
        self._model = model

        # Create the form for displaying the list of contacts.
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        # Allows the user to enter a filename and a contact
        layout.add_widget(Text("Filename:", "filename",))
        layout.add_widget(Text("Contact:", "contact",))

        layout.add_widget(Text("Birthday:", "birthday", disabled=True))
        layout.add_widget(Text("Anniversary:", "anniversary", disabled=True))
        layout.add_widget(TextBox(
            Widget.FILL_FRAME, "Other Properties:", "other_props", as_string=True, line_wrap=True, disabled=True))
        
        layout2 = Layout([1, 1, 1, 1])
        self.add_layout(layout2)
        layout2.add_widget(Button("OK", self._ok), 0)
        layout2.add_widget(Button("Cancel", self._cancel), 3)
        
        self.fix()

    def reset(self):
        # Do standard reset to clear out form, then populate with new data.
        super(AddView, self).reset()
        self.data = {
            "filename": "",
            "contact": "",
            "birthday": "",
            "anniversary": "",
            "other_props": ""
        }

    def _ok(self):
        self.save()
        self._model.add(self.data)
        raise NextScene("Main")

    @staticmethod
    def _cancel():
        raise NextScene("Main")

# Displays the database queries frame
class DBView(Frame):
    def __init__(self, screen, model):
        super(DBView, self).__init__(screen,
                                            screen.height * 2 // 3,
                                            screen.width * 2 // 3,
                                            hover_focus=True,
                                            can_scroll=True,
                                            title="DB Queries",
                                            reduce_cpu=True)

        self.model = model

        # Create the form with buttons for the two queries
        layout = Layout([100], fill_frame=True)
        self.add_layout(layout)

        # Create an empty ListBox widget that will be populated dynamically
        self._list_view = ListBox(
            Widget.FILL_FRAME,
            [],
            name="contact_list",
            add_scroll_bar=True
        )
        
        # Add the ListBox directly to the main layout
        layout.add_widget(self._list_view)

        # Add buttons for the two queries
        self._display_all_button = Button("Display all contacts", self._display_all)
        self._find_june_button = Button("Find contacts born in June", self._find_june)
        self._cancel_button = Button("Cancel", self._cancel)

        layout2 = Layout([1, 1, 1])
        self.add_layout(layout2)

        layout2.add_widget(self._display_all_button, 0)
        layout2.add_widget(self._find_june_button, 1)
        layout2.add_widget(self._cancel_button, 2)

        self.fix()

    # If the user clicks the display all contacts query
    def _display_all(self):
        # Calls a method to retrieve all contacts from the database
        contacts = self.model.get_all_contacts()

        list_data = []
        for contact in contacts:
            # Formats the dates
            birthday = contact['birthday'].strftime("%Y-%m-%d %H:%M:%S") if contact['birthday'] else "N/A"
            anniversary = contact['anniversary'].strftime("%Y-%m-%d %H:%M:%S") if contact['anniversary'] else "N/A"
            
            # Creates a list of all required information
            contact_list = f"ID: {contact['contact_id']} Name: {contact['name']} Birthday: {birthday} Anniversary: {anniversary} File: {contact['file_name']}"

            list_data.append((contact_list, contact['contact_id']))

        # Updates the ListBox with the new data
        self._list_view.options = list_data
        self._list_view.reset()

        # Refreshes the view to display the listbox
        self.fix()

    # If the user clicks the find all contacts in june query
    def _find_june(self):
        # Calls the method to retrieve contacts born in June
        contacts = self.model.find_all_june()

        list_data = []
        for contact in contacts:
            # Formats the birthday
            birthday = contact['birthday'].strftime("%Y-%m-%d %H:%M:%S") if contact['birthday'] else "N/A"

            # Creates a list of all required information
            contact_list = f"Name: {contact['name']} Birthday: {birthday}"

            list_data.append((contact_list, contact['name']))

        # Updates the ListBox with the new data
        self._list_view.options = list_data
        self._list_view.reset()

        # Refreshes the view to display the updated listbox
        self.fix()

    @staticmethod
    def _cancel():
        raise NextScene("Main")

def demo(screen, scene):
    scenes = [
        Scene([LoginView(screen)], -1, name="Login"),
        Scene([ListView(screen, contacts)], -1, name="Main"),
        Scene([ContactView(screen, contacts)], -1, name="Edit Contact"),
        Scene([AddView(screen, contacts)], -1, name="Add Contact"),
        Scene([DBView(screen, contacts)], -1, name="DB Queries"),
    ]

    screen.play(scenes, stop_on_resize=True, start_scene=scene, allow_int=True)

_db_connection = None

contacts = ContactModel()

last_scene = None
while True:
    try:
        Screen.wrapper(demo, catch_interrupt=True, arguments=[last_scene])
        sys.exit(0)
    except ResizeScreenError as e:
        last_scene = e.scene