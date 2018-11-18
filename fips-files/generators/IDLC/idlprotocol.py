import IDLC.idltypes as IDLTypes
import IDLC.idldocument as IDLDocument
import genutil as util

def Capitalize(s):
    return s[:1].upper() + s[1:]

#------------------------------------------------------------------------------
##
#
def WriteMessageDeclarations(f, document):
    for messageName, message in document["messages"].items():
        if not "fourcc" in message:
            util.fmtError('Message FourCC is required. Message "{}" does not have a fourcc!'.format(messageName))
        fourcc = message["fourcc"]

        templateArgs = ""
        messageParams = ""
        sendArgs = ""
        i = 1
        numArgs = len(document["messages"][messageName]["args"])
        for argName, T in document["messages"][messageName]["args"].items():
            typeString = IDLTypes.GetTypeString(T)
            templateArgs += typeString
            messageParams += "const {}& {}".format(typeString, argName)
            sendArgs += argName
            if i < numArgs:
                templateArgs += ", "
                messageParams += ", "
                sendArgs += ", "
            i += 1

        f.InsertNebulaDivider()
        f.WriteLine("""class {MSG} : public Game::Message<{MSG}, {TemplateArguments}>
{{
public:
    {MSG}() = delete;
    ~{MSG}() = delete;
    constexpr static const char* GetName() {{ return \"{MSG}\"; }};
    constexpr static const uint GetFourCC()	{{ return \'{FOURCC}\'; }};
    static void Send({MessageParameters})
    {{
        auto instance = Instance();
        SizeT size = instance->callbacks.Size();
        for (SizeT i = 0; i < size; ++i)
            instance->callbacks.Get<1>(i)({SendArguments});
    }}
    static void Defer(MessageQueueId qid, {MessageParameters})
    {{
        auto instance = Instance();
        SizeT index = Ids::Index(qid.id);
        auto i = instance->messageQueues[index].Alloc();
        instance->messageQueues[index].Set(i, {SendArguments});
    }}
}};
        """.format(
            MSG=messageName,
            TemplateArguments=templateArgs,
            FOURCC=fourcc,
            MessageParameters=messageParams,
            SendArguments=sendArgs,

        ))



def WriteMessageImplementation(f, document):
    # We need to set all messages within the same module at the same time.
    f.WriteLine("PYBIND11_EMBEDDED_MODULE({}, m)".format(f.fileName))
    f.WriteLine("{")
    f.IncreaseIndent()
    f.WriteLine('m.doc() = "namespace {}";'.format(IDLDocument.GetNamespace(document)))
    for messageName, message in document["messages"].items():
        if not "export" in message or message["export"] == True:

            messageDescription = ""
            if "description" in message:
                messageDescription = message["description"]

            f.WriteLine('m.def("{MSG}", &{MSG}::Send, "{MessageDescription}");'.format(
                MSG=messageName,
                MessageDescription=messageDescription
            ))
    f.DecreaseIndent()
    f.WriteLine("}")