namespace Nebula
{
    namespace Game
    {
        public interface Msg
        {
            // Empty
        }

        public interface IMessageHandler<T> where T : Msg
        {
            void OnMessage(in T message);
        }
    }
}
