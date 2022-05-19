using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.IO.Ports;

namespace freETarget.comms {

    public abstract class aCommModule {

        // Declare the delegate (if using non-generic pattern).
        public delegate void CommEventHandler(object sender, CommEventArgs e);

        // Declare the event.
        public virtual event CommEventHandler CommDataReceivedEvent;

        // Declare the event.
        public virtual event CommEventHandler CommDisconnectedEvent;

        public abstract void sendData(string text);

        public abstract void open(OpenParams value);

        public abstract void close();

        public abstract string getCommInfo();

        // Wrap the event in a protected virtual method
        // to enable derived classes to raise the event.
        protected virtual void RaiseDataReceivedEvent(string text) {
            // Raise the event in a thread-safe manner using the ?. operator.
            CommDataReceivedEvent?.Invoke(this, new CommEventArgs(text));
        }

        protected virtual void RaiseDisconnectedEvent(string text) {
            // Raise the event in a thread-safe manner using the ?. operator.
            CommDisconnectedEvent?.Invoke(this, new CommEventArgs(text));
        }
    }

    public class CommEventArgs {
        public CommEventArgs(string text) { Text = text; }
        public string Text { get; } // readonly
    }

    public interface OpenParams { }
}
