import gi, os, re, pwd, datetime, threading, socket, struct, subprocess
gi.require_version('Gtk', '3.0')
from gi.repository import Gtk
from gi.repository import GObject

unames = []
uNameOut = ""
directoryOut = ""
searchGiven = False
searchParameter = ""
stop = False
directories = []

historical = []
histcons=[]

nethist = False
hist = False
blocked = []
tempItem = ""
realTimeInterval = ""
histTimeInterval = ""

fileRegex = re.compile("^[0-9]+$")
#ignoreDev = re.compile("^/dev[a-zA-Z0-9]+")
socketRegex = re.compile("^socket+")
#deletedFileRegex = re.compile("(deleted)$")

class MainWindow(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title = "Monitor Application - Main Page")

        self.set_border_width(100)
        self.set_default_size(300,100)

        #listBox = Gtk.ListBox()
        #listBox.set_selection_mode(Gtk.SelectionMode.NONE)
        #self.add(listBox)

        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL, spacing = 30)
        self.add(vbox)

        #row_1 = Gtk.ListBoxRow()
        box1 = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL, spacing = 50)
        #row_1.add(box1)
        label1 = Gtk.Label("Username: ")
        self.username = Gtk.Entry()
        plusButton = Gtk.Button(label="+")
        minusButton = Gtk.Button(label="-")
        box1.pack_start(label1, True, True, 0)
        box1.pack_start(self.username, True, True, 0)
        boxTemp = Gtk.Box()
        boxTemp.pack_start(plusButton, True, True, 0)
        boxTemp.pack_start(minusButton, True, True, 0)
        plusButton.connect("clicked", self.plusClicked)
        minusButton.connect("clicked", self.minusClicked)
        box1.pack_start(boxTemp, True, True, 0)

        vbox.pack_start(box1, True, True, 0)
        #listBox.add(row_1)

        midBox = Gtk.Box()
        userStaticLabel = Gtk.Label("Users: ")
        self.userLabel = Gtk.Label("")
        midBox.pack_start(userStaticLabel, True, True, 0)
        midBox.pack_start(self.userLabel, True, True, 0)

        vbox.pack_start(midBox,  True, True, 0)

        #row_2 = Gtk.ListBoxRow()
        box2 = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL, spacing = 100)
        #row_2.add(box2)
        label2 = Gtk.Label("Monitor type: ")
        box2.pack_start(label2, True, True, 0)
        box3 = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL, spacing = 20)

        fileButton = Gtk.Button("File i/o")

        networkRow = Gtk.ListBoxRow()
        networkButton = Gtk.Button("Network i/o")

        box3.pack_start(fileButton, True, True, 0)
        box3.pack_start(networkButton, True, True, 0)

        box4 = Gtk.Box()
        realTimeLabel = Gtk.Label("Real Time Interval (secs): ")
        self.realTime = Gtk.Entry()
        historicalTimeLabel = Gtk.Label("Historical Interval (mins): ")
        self.historicalTime = Gtk.Entry()
        box4.pack_start(realTimeLabel, True, True, 0)
        box4.pack_start(self.realTime, True, True, 0)
        box4.pack_start(historicalTimeLabel, True, True, 0)
        box4.pack_start(self.historicalTime, True, True, 0)

        box2.pack_start(box3, True, True, 0)
        vbox.pack_start(box4, True, True, 0)
        vbox.pack_start(box2, True, True, 0)

        fileButton.connect("clicked", self.fileClick)
        networkButton.connect("clicked", self.netClick)

    def fileClick(self,widget):
        global uNameOut
        uNameOut = self.username.get_text()

        global stop
        stop = False

        global realTimeInterval
        realTimeInterval = self.realTime.get_text()
        global histTimeInterval
        histTimeInterval = self.historicalTime.get_text()

        dirWindow = DirectoryWindow()
        dirWindow.connect("delete-event", Gtk.main_quit)
        dirWindow.set_position(Gtk.WindowPosition.CENTER)
        dirWindow.show_all()
        Gtk.main()

    def netClick(self,widget):
        global uNameOut
        uNameOut = self.username.get_text()

        global stop
        stop = False

        global realTimeInterval
        realTimeInterval = self.realTime.get_text()
        global histTimeInterval
        histTimeInterval = self.historicalTime.get_text()

        networkWindow = NetworkWindow()
        networkWindow.connect("delete-event", networkWindow.networkQuit)
        networkWindow.set_position(Gtk.WindowPosition.CENTER)
        networkWindow.show_all()
        Gtk.main()

    def selectionClicked(self, widget, row):
        print(self.username.get_text())

        global uNameOut
        uNameOut = self.username.get_text()

        selection = row.get_children()[0].get_text()
        print("clicked " + selection)

        if (selection == "File i/o"):
            global stop
            stop = False
            dirWindow = DirectoryWindow()
            dirWindow.connect("delete-event", Gtk.main_quit)
            dirWindow.set_position(Gtk.WindowPosition.CENTER)

            dirWindow.show_all()
            Gtk.main()

        elif (selection == "Network i/o"):

            global stop
            stop = False
            networkWindow = NetworkWindow()
            networkWindow.connect("delete-event", networkWindow.networkQuit)
            networkWindow.set_position(Gtk.WindowPosition.CENTER)

            networkWindow.show_all()
            Gtk.main()
            # window.destroy()

    def plusClicked(self, widget):
        global unames
        unames.append(self.username.get_text())

        if self.userLabel.get_text() == "":
            self.userLabel.set_text(self.username.get_text())
        else:
            self.userLabel.set_text(self.userLabel.get_text() + ", " + self.username.get_text())

        self.username.set_text("")
        self.username.grab_focus()

    def minusClicked(self, widget):
        global unames
        unames.pop()
        self.userLabel.set_text("")
        for name in unames:
            if(self.userLabel.get_text() == ""):
                self.userLabel.set_text(name)
            else:
                self.userLabel.set_text(self.userLabel.get_text() + ", " + name)

class DirectoryWindow(Gtk.Window):

    def __init__(self):
        Gtk.Window.__init__(self, title="Monitor Application - Directory Page")
        self.set_border_width(150)
        self.set_default_size(500, 400)

        vbox = Gtk.Box(orientation = Gtk.Orientation.VERTICAL, spacing = 10)
        self.add(vbox)

        hbox1 = Gtk.Box(orientation = Gtk.Orientation.HORIZONTAL, spacing = 10)
        uname = Gtk.Label("Username: ")

#        global uNameOut
        global unames
        self.uNameEntered = Gtk.Label()
        for name in unames:
            if self.uNameEntered.get_text() == "":
                self.uNameEntered.set_text(name)
            else:
                self.uNameEntered.set_text(self.uNameEntered.get_text() + ", " + name)

        #uNameOut = self.uNameEntered

        hbox1.pack_start(uname, True, True, 0)
        hbox1.pack_start(self.uNameEntered, True, True, 0)

        vbox.pack_start(hbox1, True, True, 0)

        hbox2 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        directoryLabel = Gtk.Label("Directory / File to monitor: ")
        self.file = Gtk.Entry()
        self.file.set_text("/home/karanm1992/k")
        hbox2.pack_start(directoryLabel, True, True, 0)
        hbox2.pack_start(self.file, True, True, 0)

        vbox.pack_start(hbox2, True, True, 0)

        hbox4 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        self.dirLabel = Gtk.Label("")
        hbox4.pack_start(self.dirLabel, True, True, 0)

        vbox.pack_start(hbox4, True, True, 0)

        hbox3 = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL, spacing=10)
        addButton = Gtk.Button(label="ADD")
        addButton.connect("clicked", self.addClicked)
        self.delButton = Gtk.Button(label="DELETE")
        self.delButton.set_sensitive(False)
        self.delButton.connect("clicked", self.delClicked)
        startButton = Gtk.Button(label="START")
        startButton.connect("clicked", self.startClicked)
        startButton.set_size_request(20,20)
        hbox3.pack_start(addButton, True, True, 0)
        hbox3.pack_start(self.delButton, True, True, 0)
        hbox3.pack_start(startButton, True, True, 0)

        vbox.pack_start(hbox3, True, True, 0)



    def delClicked(self,widget):
        global directories
        directories.pop()

        self.dirLabel.set_text("")
        for dir in directories:
            if self.dirLabel.get_text() == "":
                self.dirLabel.set_text(dir)
            else:
                self.dirLabel.set_text(self.dirLabel.get_text() + ", " + dir)

        if self.dirLabel.get_text() == "":
            self.delButton.set_sensitive(False)

    def addClicked(self,widget):
        self.delButton.set_sensitive(True)
        global directories
        directories.append(self.file.get_text())

        if (self.dirLabel.get_text() == ""):
            self.dirLabel.set_text(self.file.get_text())
        else:
            self.dirLabel.set_text(self.dirLabel.get_text() + ", " + self.file.get_text())

       # print (directories)
        self.file.set_text("")
        self.file.grab_focus()

    def startClicked(self, widget):
        global directoryOut
        directoryOut = self.file.get_text()
        if directoryOut == "":
            directoryOut = '/'

        global uNameOut
        uNameOut = self.uNameEntered.get_text()

        global stop
        stop = False

        fileWindow = FileMonitorWindow()
        fileWindow.connect("delete-event", fileWindow.fileQuit)
        fileWindow.set_position(Gtk.WindowPosition.CENTER)
        fileWindow.show_all()
        Gtk.main()
        #self.destroy()

def timeout(selfPtr):
    print ("calling refresh")
    global stop
    print ("stop = " + str(stop))
    if (not stop):
        selfPtr.refresh(True)
    return 1

class NetworkWindow(Gtk.Window):

    def networkQuit(self, widget, data):
        print("in network quit")

        global stop
        stop = True
        print ("setting stop to true")
        #while(self.t.finished):
        self.t.join()
        self.t.cancel()

        global searchGiven
        searchGiven = False

        Gtk.main_quit()

    def getCons(self):
        global uNameOut
        print("username - " + uNameOut)

        procDirs = os.listdir("/proc")
        userProcs = []

        cons = []

        #tcpFile = open("/proc/net/tcp")
        #udpFile = open("/proc/net/udp")

        #  uNameRegex = re.compile(uNameOut + '+')
        for dir in procDirs:  # for all files in /proc
            if not os.path.exists("/proc/" + dir):  # check if process still exists
                continue

            if re.match(fileRegex, dir):# and \
                  #  pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0].startswith(uNameOut):
                userProcs.append(dir)

                username = pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0]
                # print (userProcs)
                # print("user proc - " + dir)

                # monitorFiles = {"abc"}

                global unames
                match = False
                for name in unames:
                    if username.startswith(name):
                        match = True

                if (not match):
                    continue

                for root, dirs, files in os.walk("/proc/" + dir + "/fd"):  # , followlinks = True   in every process' fd

                    for name in files:
                        socketFound = False
                        # print (os.path.join(root, name))
                        if not os.path.exists("/proc/" + dir + "/fd"):  # check if process still exists
                            continue
                        else:

                            if (not os.path.islink(os.path.join(root, name))):  # check if link still exists
                                continue
                                # else:
                                #  print("link exists")

                            link = os.readlink(os.path.join(root, name))
                            # print ("LINK - " + link)


                            if re.match(socketRegex, link):
                                # print ("socket" + link[8:-1])
                                # print (dir)
                                tcpFile = open("/proc/net/tcp")
                                for tcpLine in tcpFile:
                                    #print tcpLine
                                    tcpOutput = [tcpLine.split()]
                                    for tcpWord in tcpOutput:
                                        #print ("comparing " + link[8:-1] + " to " + tcpWord[9])
                                        if (link[8:-1] == tcpWord[9]):
                                            socketFound = True
                                            localAddress = tcpWord[1]
                                            remoteAddress = tcpWord[2]
                                            break
                                            # print (tcpWord[1] + "  " + tcpWord[2])

                                if (not socketFound):
                                    udpFile = open("/proc/net/udp")
                                    for udpLine in udpFile:
                                        udpOutput = [udpLine.split()]
                                        for udpWord in udpOutput:
                                           # print ("comparing " + link[8:-1] + " to " + udpWord[9])
                                            if (link[8:-1] == udpWord[9]):
                                                # print (udpWord[1] + "  " + udpWord[2])
                                                socketFound = True
                                                localAddress = udpWord[1]
                                                remoteAddress = udpWord[2]
                                                break

                            if (socketFound):
                                # print ("Username - " + pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0]\
                                #                               + " user id - " + dir\
                                #                             + " localAddr - " + localAddress\
                                #                               + " remoteAddr - " + remoteAddress\
                                #                               + " timestamp")

                                cmdFile = open("/proc/" + dir + "/cmdline")

                                for line in cmdFile:
                                    # print (line)
                                    cmd = line

                                local = [localAddress.split(":")]
                                localIp = socket.inet_ntoa(struct.pack("<L", int(local[0][0], 16) ))
                                localPort = int(local[0][1], 16)

                                try:
                                    localHost = socket.gethostbyaddr(localIp)[0]
                                except:
                                    localHost = localIp

                                remote = [remoteAddress.split(":")]
                                remoteIp = socket.inet_ntoa(struct.pack("<L", int(remote[0][0], 16)))
                                remotePort = int(local[0][1], 16)

                                try:
                                    remoteHost = socket.gethostbyaddr(remoteIp)[0]
                                except:
                                    remoteHost = localIp


                               # print("stating" + os.path.join(root, name))
                                # print(os.path.getatime(os.path.join(root, name)))
                                # print (os.stat(os.path.join(root, name)).st_mtime)

                                tuple = [(dir, cmd, username, \
                                             localHost+":"+str(localPort), remoteHost+":"+str(remotePort))]

                                cons.append(tuple[0])
                                exists = False
                                global histcons
                                for entry in histcons:
                                    if tuple[0] == entry:
                                        exists = True
                                        break

                                if (not exists):
                                    histcons.append(tuple[0])

        searchView = []

        if (searchGiven):
            print ("running search")
            for i, tuple in enumerate(cons):
                for item in tuple:
                    if searchParameter in str(item):
                        # print ("paramFound in - " + str(item))
                        searchView.append(cons[i])
                        break

            return searchView
        else:
            return cons


    def refresh(self, start):
        print ("in refresh")
        #print (self.con_list_store[0])

        cons = []
        cons = self.getCons()
#        print (cons)


        global nethist
        if(not nethist):
            self.con_list_store.clear()
            print ('reached here')

            try:
                for connection in cons:
                    self.con_list_store.append(list(connection))
            except:
                print ("****LIST STORE EXCEPTION****")
                #print Exception(e.args)


            try:
                self.treeView.set_model(self.con_list_store)
            except:
                print ("****EXCEPTION****")
                #print Exception(e.args)

        if(start):
            self.t.cancel()
            global realTimeInterval
            t = threading.Timer(float(realTimeInterval), timeout, [self])
            t.start()


    def __init__(self):
        Gtk.Window.__init__(self, title="Monitor Application - Network Page")
        self.set_border_width(50)
        self.set_default_size(1050, 500)
        print ("network window")

        cons = self.getCons()
       # print(cons)
        self.con_list_store = Gtk.ListStore(str, str, str, str, str)

        for connection in cons:
            self.con_list_store.append(list(connection))

        self.treeView = Gtk.TreeView(self.con_list_store)

        for i, col_title in enumerate(["PID", "Process Name", "Username", "LocalAddr", "RemoteAddr"]):
            renderer = Gtk.CellRendererText()
            column = Gtk.TreeViewColumn(col_title, renderer, text=i)
            column.set_sort_column_id(i)
            self.treeView.append_column(column)

        self.tree_selection = self.treeView.get_selection()
        #   tree_selection.set_mode(Gtk.SELECTION.SELECTION_MULTIPLE)
        self.tree_selection.connect("changed", self.onSelectionChanged)

        self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.search = Gtk.Entry()
        listBox = Gtk.ListBox()

        self.sw = Gtk.ScrolledWindow()
        self.sw.add_with_viewport(self.treeView)
        self.box.pack_start(self.sw, True, True, 0)
        # box.pack_start(self.search, True, True, 0)


        ##############################

        histButton = Gtk.Button(label="History")
        histButton.connect("clicked", self.showHist)
        realButton = Gtk.Button(label="Real Time")
        realButton.connect("clicked", self.showReal)
        blockAddButton = Gtk.Button(label="Add to Block List")
        blockAddButton.connect("clicked", self.addToBlockList)
        blockButton = Gtk.Button(label="Block")
        blockButton.connect("clicked", self.blockProcs)
        unblockButton = Gtk.Button(label="Unblock")
        unblockButton.connect("clicked", self.unblockProcs)

        ##############################


#        global blocked
 #       file = open("/home/karanm1992/block.txt")
  #      for line in file:
   #         blocked.append(line)

    #    for file in blocked:
     #       self.blockLabel.set_text(self.blockLabel.get_text() + file)

        listBoxRow = Gtk.ListBoxRow()
        listBoxRow.add(self.search)
        listBox.add(listBoxRow)

        listBoxButtonRow = Gtk.ListBoxRow()

        buttonBox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        buttonBox.pack_start(histButton, True, True, 0)
        buttonBox.pack_start(realButton, True, True, 0)
        buttonBox.pack_start(blockAddButton, True, True, 0)
        buttonBox.pack_start(blockButton, True, True, 0)
        buttonBox.pack_start(unblockButton, True, True, 0)

        listBoxButtonRow.add(buttonBox)

        listBox.add(listBoxButtonRow)

        self.staticBlock = Gtk.Label("Blocked directories are: ")
        self.blockLabel = Gtk.Label("")

        blockRow = Gtk.ListBoxRow()
        blockBox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        blockBox.pack_start(self.staticBlock, True, True, 0)
        blockBox.pack_start(self.blockLabel, True, True, 0)

        blockRow.add(blockBox)

        listBox.add(blockRow)

        self.search.connect("focus-out-event", self.searchEntered)

        self.sw.show()

        self.box.pack_start(listBox, True, True, 0)
        self.add(self.box)


        #timeout(self)

        global realTimeInterval
        self.t = threading.Timer(float(realTimeInterval), timeout, [self])
        self.t.start()

    def showReal(self, widget):
        print ("calling real")
        self.refresh(True)
        global stop
        stop = False
        global nethist
        nethist = False

    def showHist(self, widget):
        print ("in net hist")

        global histcons

        self.con_list_store.clear()
        print ('reached here')

        for connection in histcons:
            self.con_list_store.append(list(connection))

        self.treeView.set_model(self.con_list_store)

        global nethist
        nethist = True

    def onSelectionChanged(self, data):
        global tempItem
        value = ""
        (model, pathlist) = self.treeView.get_selection().get_selected_rows()
        for path in pathlist:
            tree_iter = model.get_iter(path)
            value = model.get_value(tree_iter, 4)
            #print value

        tempItem = value

    def addToBlockList(self, widget):
        print ("in changed")

        global tempItem
        exists = False
        global blocked
        for entry in blocked:
            if tempItem == entry:
                exists = True
                break

        if(not exists):
            blocked.append(tempItem)
            f = open("/home/karanm1992/block_net.conf", "a+")

            f.write(socket.gethostbyname(tempItem.split(":")[0])+ "\n")
            if self.blockLabel.get_text() == "":
                self.blockLabel.set_text(tempItem)
            else:
                self.blockLabel.set_text(self.blockLabel.get_text() + ", " + tempItem)

    def blockProcs(self,widget):
        print("block procs")
        subprocess.call(['./script.sh'])

    def unblockProcs(self,widget):
        print("unblock procs")
        subprocess.call(['./unblock.sh'])
        global blocked
        blocked = []
        self.blockLabel.set_text("")
        open("/home/karanm1992/block_net.conf", 'w').close()

    def searchEntered(self, widget, event):
        global searchParameter
        searchParameter = self.search.get_text()

        global searchGiven
        if searchParameter != "":
            searchGiven = True

        print (searchParameter)
        self.refresh(False)

class FileMonitorWindow(Gtk.Window):

    def fileQuit(self, widget, data):
        print("in file quit")

        global directories
        directories = []

        global stop
        stop = True
        print ("setting stop to true")
        # while(self.t.finished):
        self.t.join()
        self.t.cancel()

        global searchGiven
        searchGiven = False

        Gtk.main_quit()

    def getFiles(self):
        global uNameOut
        print("username - " + uNameOut)
        # uNameRegex = re.compile(uNameOut+'+')

        global directoryOut
        print("directory - " + directoryOut)

        global directories
        print ("directories -")
        print(directories)
        # userFiles = os.listdir(directoryOut)

        #directoryReg = re.compile(directoryOut + "[a-zA-Z\d\/\ ]*")

        # for file in userFiles:
        #   print (directoryOut + "/" + file)

        procDirs = os.listdir("/proc")

        userProcs = []
        filesView = []

        # re.match(uNameRegex, pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0]): #if file is no. and user is as given
        for dir in procDirs:  # for all files in /proc
            if not os.path.exists("/proc/" + dir):  # check if process still exists
                continue

            if re.match(fileRegex, dir):
                   # pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0].startswith(uNameOut):
                userProcs.append(dir)

                username = pwd.getpwuid(os.stat("/proc/" + dir).st_uid)[0]
                # print (userProcs)
                # print ("user proc - " + dir)

                global unames
                match = False
                for name in unames:
                    if username.startswith(name):
                        match = True

                if(not match):
                    continue

                for root, dirs, files in os.walk("/proc/" + dir + "/fd"):  # , followlinks = True   in every process' fd
                    for name in files:
                        # print("file - " + os.path.join(root, name))
                        # print(os.readlink(os.path.join(root, name)))
                        if not os.path.exists("/proc/" + dir + "/fd"):  # check if process still exists
                            continue
                        else:
                            # print("root - " + root + " name - " + str(name))
                            # name = str(name)

                            # for file in userFiles:  #ignore the pattern given


                            if (not os.path.islink(os.path.join(root, name))):  # check if link still exists
                                continue

                            link = os.readlink(os.path.join(root, name))

                            if (link.endswith("(deleted)") or link.startswith("/dev") or \
                                        link.startswith("socket") or link.startswith("anon") \
                                        or link.startswith("pipe")):
                                # print ("ignoring - " + link)
                                continue

                            check = False

                            for d in directories:
                               # print(link + "   " + d )
                                if link.startswith(d):
                                    check = True
                                    break
                                    #print("----------------------------" + link + " pid = " + dir)

                            if check:
                                cmdline = open("/proc/" + dir + "/cmdline")
                                cmd = cmdline.read()

                                tuple = [(int(dir), cmd, link, \
                                                  username, \
                                                  datetime.datetime.fromtimestamp(int(os.stat(link).st_atime)). \
                                                  strftime('%Y-%m-%d %H:%M:%S'))]

                                filesView.append(tuple[0])
                                exists = False
                                global historical
                                for entry in historical:
                                    if tuple[0] == entry:
                                        exists = True
                                        break

                                if (not exists):
                                    historical.append(tuple[0])

        global searchGiven
        print "search - " + str(searchGiven)
        global searchParameter
        print "search - " + searchParameter


        searchView = []
        if not searchParameter == "":
            print ("running search")
            for tuple in filesView:
                for item in tuple:
                    if searchParameter in str(item):
                        #print ("paramFound in - " + str(item))
                        searchView.append(tuple)
                        break

            return searchView
        else:
            return filesView



    def refresh(self, start):
        print ("in refresh")
        #print (self.con_list_store[0])

        filesView = []
        filesView = self.getFiles()
#        print (filesView)

        print ("reached here")

        global hist
        if (not hist):
            #self.files_list_store.clear()
            self.files_list_store = Gtk.ListStore(int, str, str, str, str)

            for files in filesView:
                self.files_list_store.append(list(files))

            self.treeView.set_model(self.files_list_store)

      #  self.hist_list_store = Gtk.ListStore(int, str, str, str, str)

#        global historical
#        historical = sorted(historical, key=lambda x: (x[2], x[3], x[4]))
#        for files in historical:
#            self.hist_list_store.append(list(files))##

        #self.hist_treeView.set_model(self.hist_list_store)

        if(start):
            global realTimeInterval
            t2 = threading.Timer(float(realTimeInterval), timeout, [self])
            t2.start()

    def showReal(self, widget):
        print ("calling real")
        self.refresh(True)
        global stop
        stop = False
        global hist
        hist = False

    def showHist(self, widget):
        print ("in hist")

        # self.files_list_store.clear()
        global historical
        self.files_list_store = Gtk.ListStore(int, str, str, str, str)

        for files in historical:
            self.files_list_store.append(list(files))

        self.treeView.set_model(self.files_list_store)

        global hist
        hist = True

    def onSelectionChanged(self, data):
        global tempItem
        value = ""
        (model, pathlist) = self.treeView.get_selection().get_selected_rows()
        for path in pathlist:
            tree_iter = model.get_iter(path)
            value = model.get_value(tree_iter, 2)
            print value
        tempItem = value

    def addToBlockList(self, widget):
        print ("in changed")

        global tempItem
        exists = False
        global blocked
        for entry in blocked:
            if tempItem == entry:
                exists = True
                break

        if(not exists):
            blocked.append(tempItem)
            f = open("/home/karanm1992/block_files.conf", "a+")
            f.write(tempItem + "\n")

            if self.blockLabel.get_text() == "":
                self.blockLabel.set_text(tempItem)
            else:
                self.blockLabel.set_text(self.blockLabel.get_text() + ", " + tempItem)

    def blockProcs(self,widget):
        print("block procs")
        subprocess.call(['./script.sh'])

    def unblockProcs(self,widget):
        print("unblock procs")
        subprocess.call(['./unblock.sh'])
        global blocked
        blocked = []
        self.blockLabel.set_text("")
        open("/home/karanm1992/block_file.conf", 'w').close()

    def __init__(self):
        Gtk.Window.__init__(self, title="Monitor Application - File Monitor Page")
        self.set_border_width(50)
        self.set_default_size(750, 500)
        #self.set_frame_dimensions(top = 100)
        print("File monitor window")

        filesView = self.getFiles()
        print("files -")
        #print(filesView)

        self.files_list_store = Gtk.ListStore(int, str, str, str, str)

        for file in filesView:
            self.files_list_store.append(list(file))

        self.treeView = Gtk.TreeView(self.files_list_store)

        for i, col_title in enumerate(["PID", "Process Name", "File Name", "User name", "Timestamp"]):
            renderer = Gtk.CellRendererText()
            column = Gtk.TreeViewColumn(col_title, renderer, text=i)
            column.set_sort_column_id(i)
            self.treeView.append_column(column)

        self.box = Gtk.Box(orientation=Gtk.Orientation.VERTICAL, spacing=0)
        self.search = Gtk.Entry()
        listBox = Gtk.ListBox()

        self.sw = Gtk.ScrolledWindow()
        self.sw.add_with_viewport(self.treeView)
        self.box.pack_start(self.sw, True, True, 0)
        # box.pack_start(self.search, True, True, 0)

        self.tree_selection = self.treeView.get_selection()
        #   tree_selection.set_mode(Gtk.SELECTION.SELECTION_MULTIPLE)
        self.tree_selection.connect("changed", self.onSelectionChanged)


        ##############################

        histButton = Gtk.Button(label="History")
        histButton.connect("clicked", self.showHist)
        realButton = Gtk.Button(label="Real Time")
        realButton.connect("clicked", self.showReal)
        blockAddButton = Gtk.Button(label="Add to Block List")
        blockAddButton.connect("clicked", self.addToBlockList)
        blockButton = Gtk.Button(label="Block")
        blockButton.connect("clicked", self.blockProcs)
        unblockButton = Gtk.Button(label="Unblock")
        unblockButton.connect("clicked", self.unblockProcs)

        ##############################


        #        global blocked
        #       file = open("/home/karanm1992/block.txt")
        #      for line in file:
        #         blocked.append(line)

        #    for file in blocked:
        #       self.blockLabel.set_text(self.blockLabel.get_text() + file)

        listBoxRow = Gtk.ListBoxRow()
        listBoxRow.add(self.search)
        listBox.add(listBoxRow)

        listBoxButtonRow = Gtk.ListBoxRow()

        buttonBox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        buttonBox.pack_start(histButton, True, True, 0)
        buttonBox.pack_start(realButton, True, True, 0)
        buttonBox.pack_start(blockAddButton, True, True, 0)
        buttonBox.pack_start(blockButton, True, True, 0)
        buttonBox.pack_start(unblockButton, True, True, 0)

        listBoxButtonRow.add(buttonBox)

        listBox.add(listBoxButtonRow)

        self.staticBlock = Gtk.Label("Blocked directories are: ")
        self.blockLabel = Gtk.Label("")

        blockRow = Gtk.ListBoxRow()
        blockBox = Gtk.Box(orientation=Gtk.Orientation.HORIZONTAL)

        blockBox.pack_start(self.staticBlock, True, True, 0)
        blockBox.pack_start(self.blockLabel, True, True, 0)

        blockRow.add(blockBox)

        listBox.add(blockRow)

        self.search.connect("focus-out-event", self.searchEntered)

        self.sw.show()

        self.box.pack_start(listBox, True, True, 0)
        self.add(self.box)

        global blocked
        file = open("/home/karanm1992/block_files.conf")
        for line in file:
            blocked.append(line)

        for file in blocked:
            self.blockLabel.set_text(self.blockLabel.get_text() + file)


        print("reloading block list")
        print(blocked)

        global realTimeInterval
        self.t = threading.Timer(float(realTimeInterval), timeout, [self])
        self.t.start()

        global histTimeInterval
        self.h = threading.Timer(float(histTimeInterval)*60.0, self.histLog, [self])
        self.h.start()

    def histLog(self, widget):
        print("writing historical data")
        global historical
        temp = historical
        written = []
        f = open("/home/karanm1992/hist_files.log", "a+")

        for item in temp:
            maxItem = item
            for each in temp:
                if(item[2] == each[2] and item[3] == each[3]):
                    print("item" + str(item) + " " + str(each))
                    print("checking time for " + str(maxItem) + " and " + str(each))
                    if(each[4] > maxItem[4]):
                        maxItem = each

            exists = False
            for check in written:
                if maxItem == check:
                    exists = True

            if(not exists):
                print("writing " + str(maxItem))
                f.write(str(maxItem) + "\n")
                written.append(maxItem)
          #      if (item[0] == each[0]):
         #   print(str(item[0]) + "   " + str(item[2]) + "  " + str(item[3]))



    def searchEntered(self, widget, event):
        global searchParameter
        searchParameter = self.search.get_text()

        global searchGiven
        if searchParameter != "":
            searchGiven = True

        print (searchParameter)
        self.refresh(False)
        #print (filesView)


window  = MainWindow()
window.connect("delete-event", Gtk.main_quit)
window.set_position(Gtk.WindowPosition.CENTER)
window.show_all()
Gtk.main()