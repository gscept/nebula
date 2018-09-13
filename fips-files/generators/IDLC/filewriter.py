import os, platform

class FileWriter:
    def __init__(self):
        self.tab = "    "
        self.tabSize = 4
        self.currentIndent = 0
        self.filePath = ""
        self.file = None
        self.line = 0;
        self.column = 0;
        self.eol = "\n"

    #------------------------------------------------------------------------------
    ##
    #
    def Open(self, input) :
        self.filePath = input
        self.file = open(self.filePath, 'w')
        self.line = 0
        self.currentIndent = 0
        self.column = 0

    #------------------------------------------------------------------------------
    ##
    #
    def IncreaseIndent(self):
        self.currentIndent += 1

    #------------------------------------------------------------------------------
    ##
    #
    def DecreaseIndent(self):
        self.currentIndent = max(self.currentIndent - 1, 0)

    #------------------------------------------------------------------------------
    ##
    #
    def SetIndent(self, value):
        self.currentIndent = max(value, 0)

    #------------------------------------------------------------------------------
    ##
    #
    def WriteLine(self, string):
        self.Write(string)
        self.file.write(self.eol)
        self.line += 1
        self.column = 0

    #------------------------------------------------------------------------------
    ##
    #
    def InsertNebulaDivider(self):
        self.WriteLine("")
        self.WriteLine("//------------------------------------------------------------------------------")
        self.WriteLine("/**")
        self.WriteLine("*/")

    #------------------------------------------------------------------------------
    ##
    #
    def InsertNebulaComment(self, comment):
        self.WriteLine("")
        self.WriteLine("//------------------------------------------------------------------------------")
        self.WriteLine("/**")
        self.Write("    ")
        self.WriteLine(comment)
        self.WriteLine("*/")

    #------------------------------------------------------------------------------
    ##
    #
    def Write(self, string):
        # If we're already in the middle of a line we don't add any indentation.        
        if self.column == 0:
            for i in range(self.currentIndent):
                self.file.write(self.tab)
                self.column += self.tabSize

        numEOLs = string.count(self.eol)
        lastEOL = string.rfind(self.eol)

        # If there's EOL escape characters in the middle of the string, we might
        # need to add indentations.
        if lastEOL != -1:
            if self.currentIndent > 0:
                indentCorrectedEOL = "\n"
                for i in range(self.currentIndent):
                    indentCorrectedEOL += self.tab
                # If the last EOL is at the end of the string, don't insert any tabs in the end
                if lastEOL == len(string):
                    string = string.replace("\n", indentCorrectedEOL, numEOLs - 1)
                else:
                    string = string.replace("\n", indentCorrectedEOL)

        self.file.write(string)
        self.column += len(string)

        # Adjust line and column if the string contains eol escape character
        if lastEOL != -1:
            self.line += string.count(self.eol)
            self.column = len(string) - (lastEOL + 1)

    def Close(self):
        self.file.close()