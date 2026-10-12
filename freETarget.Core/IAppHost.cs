namespace freETarget {

    /**
     * what the UI-independent code needs from the application it runs in: the log and the ways to talk to the user.
     * implemented by the main window
     */
    public interface IAppHost {

        void log(string s);

        void displayMessage(string text, bool showToolTip);

        void showError(string text, string title);

        void showWarning(string text, string title);

        /** asks the user an OK/Cancel question. returns true for OK */
        bool confirm(string text, string title);

        Event findEventByID(long id);
    }
}
