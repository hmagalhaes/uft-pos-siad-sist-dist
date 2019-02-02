import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.net.UnknownHostException;

public class ClienteArquivos {

	private static final String SERVER_ADDRESS = "127.0.0.1";
	private static final int SERVER_PORT = 12345;

	public static void main(String[] args) throws UnknownHostException, IOException {
		final String fileName = getFileName(args);
		if (isBlank(fileName)) {
			System.out.println("Por favor informe o nome do arquivo como segundo parâmetro por parâmetro");
			return;
		}

		try (Socket socket = new Socket(SERVER_ADDRESS, SERVER_PORT)) {
			System.out.println("O cliente se conectou ao servidor!");
			System.out.println("Enviando arquivo => " + fileName);

			try (InputStream is = new FileInputStream(fileName)) {
				try (OutputStream os = socket.getOutputStream()) {
					copy(is, os);
				} catch (IOException ex) {
					throw new RuntimeException("Erro ao enviar arquivo", ex);
				}
			} catch (IOException ex) {
				throw new RuntimeException("Erro ao ler arquivo => " + fileName);
			}
		} catch (UnknownHostException ex) {
			throw new RuntimeException("Servidor não está rodando em => " + SERVER_ADDRESS + ":" + SERVER_PORT, ex);
		}

		System.out.println("Arquivo enviado :D");
	}

	private static String getFileName(String[] args) {
		if (isEmpty(args)) {
			return null;
		}
		return args[0].trim();
	}

	private static void copy(final InputStream is, final OutputStream os) throws IOException {
		int buffer = 0;
		while ((buffer = is.read()) >= 0) {
			os.write(buffer);
		}
		os.flush();
	}

	private static boolean isBlank(String value) {
		if (value == null) {
			return true;
		}
		return value.trim().isEmpty();
	}

	private static boolean isEmpty(Object[] args) {
		return args == null || args.length <= 0;
	}

}