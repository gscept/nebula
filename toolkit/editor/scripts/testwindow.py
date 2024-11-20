import editor
import deargui as dg

category = "Scripting tests"

check = False
def draw():
    global check
    check = dg.checkbox("check me!", check)[1]
    if check :
        dg.text("you checked me!")